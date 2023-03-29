#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/regulator/consumer.h>
#include "../../huawei_ts_kit_algo.h"
#include "../../tpkit_platform_adapter.h"
#include "atmel.h"

#define MXT_FW_CHG_TIMEOUT 5000
#define MXT_FW_RESET_TIME 250
#define CONFIG_MXT_UPDATE_BY_OBJECT
#define OBJECT_CFG_HEAD_SIZE 16

extern struct mxt_data *mxt_core_data;
static int atmel_get_lcd_panel_info(void);
static int atmel_get_lcd_module_name(void);
//parse lcd dts for get lcd panel info
static int atmel_get_lcd_panel_info(void)
{
	struct device_node *dev_node = NULL;
	char *lcd_type = NULL;
	dev_node = of_find_compatible_node(NULL, NULL, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
	if (!dev_node) {
		TS_LOG_ERR("%s: NOT found device node[%s]!\n", __func__, LCD_PANEL_TYPE_DEVICE_NODE_NAME);
		return -EINVAL;
	}
	lcd_type = (char*)of_get_property(dev_node, "lcd_panel_type", NULL);
	if(!lcd_type){
		TS_LOG_ERR("%s: Get lcd panel type faile!\n", __func__);
		return -EINVAL ;
	}

	strncpy(mxt_core_data->lcd_panel_info, lcd_type, LCD_PANEL_INFO_MAX_LEN-1);
	TS_LOG_INFO("lcd_panel_info = %s.\n", mxt_core_data->lcd_panel_info);
	return 0;
}
//parse lcd panel info for lcd module name
static int  atmel_get_lcd_module_name(void)
{
	char temp[LCD_PANEL_INFO_MAX_LEN] = {0};
	int i = 0;
	if(!mxt_core_data) {
		TS_LOG_ERR("%s, data is NULL\n", __func__);
		return -EINVAL;
	}
	memset(mxt_core_data->lcd_module_name, 0 , MAX_STR_LEN);
	strncpy(temp, mxt_core_data->lcd_panel_info, LCD_PANEL_INFO_MAX_LEN-1);
	for(i=0;i<(LCD_PANEL_INFO_MAX_LEN -1) && (i < (MAX_STR_LEN -1));i++)
	{
		if(temp[i] == '_')
		{
			break;
		}
		mxt_core_data->lcd_module_name[i] = tolower(temp[i]);
	}
	TS_LOG_INFO("lcd_module_name = %s.\n", mxt_core_data->lcd_module_name);
	return 0;
}

/*
 *Function for enabling T37 diagnostic mode through T6.
 *
 *t37 is used to store diagnostic data which can be references, delta and so on. Which kind of data is stored is controled by byte 5 of t6.
 *this function fist set t6 byte 5 and then read t37 for the data wanted.
 *
 */
static int atmel_diagnostic_command(struct mxt_data *data, u8 cmd, u8 page, u8 index,
			   u8 num, char *buf, int interval, int interval_c)
{
	u16 reg = 0;
	u8 command_register[2] = {0};	/*2,command num*/
	s8 current_page = 0, page_cmd = 0;
	int ret = 0;
	int timeout_counter = 0, retry = 0;

	long unsigned int time_start = jiffies;

	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	reg = data->T37_address;
	ret =
	    __atmel_read_reg(data, reg, sizeof(command_register),
			   &command_register[0]);
	if (ret) {
		TS_LOG_ERR("T37 read offset 0 failed %d!\n", ret);
		return -EIO;
	}
#define T37_COMMAND_RESEND_INDEX  2
	if (command_register[0] != cmd
	    && (page == 0 && index < T37_COMMAND_RESEND_INDEX)) {
		TS_LOG_INFO
		    ("%s, cmd = %d, command_register[0] = %d ,  page = %d , index = %d\n",
		     __func__, cmd, command_register[0], page, index);
		ret = atmel_t6_command(data, MXT_COMMAND_DIAGNOSTIC, cmd, 0);
		if (ret) {
			TS_LOG_ERR
			    ("T6 Command DIAGNOSTIC cmd 0x%x failed %d!\n", cmd,
			     ret);
			return -EIO;
		}
		current_page = 0;
	} else {
		current_page = command_register[1];
		if (abs((s8) current_page - (s8) page) > (s8) page) {	/* far away with dis page*/
			ret =
			    atmel_t6_command(data, MXT_COMMAND_DIAGNOSTIC, cmd,
					   0);
			if (ret) {
				TS_LOG_ERR
				    ("T6 Command DIAGNOSTIC cmd 0x%x failed %d!\n",
				     cmd, ret);
				return -EIO;
			}
			current_page = 0;
		}
	}

	/*wait command*/
	timeout_counter = 0;
	do {
		msleep(interval);
		reg = data->T37_address;
		ret =
		    __atmel_read_reg(data, reg, sizeof(command_register),
				   &command_register[0]);
		if (ret) {
			TS_LOG_ERR("T37 read offset 0 failed %d!\n", ret);
			return -EIO;
		}

		if ((u8) command_register[0] == cmd)
			break;

		if (!interval_c)
			return -EAGAIN;

	} while (timeout_counter++ <= interval_c);

	if (timeout_counter > interval_c) {
		TS_LOG_ERR
		    ("T37 wait cmd %d page %d current page %d timeout, T37 buf(%d %d) ##1\n",
		     cmd, page, current_page, (u8) command_register[0],
		     command_register[1]);
		return -EBUSY;
	}
	/*current_page = command_register[1];*/
	retry = 0;
	while (current_page != page) {

		if (current_page > page) {
			page_cmd = MXT_T6_DEBUG_PAGEDOWN;
			current_page--;
		} else {
			page_cmd = MXT_T6_DEBUG_PAGEUP;
			current_page++;
		}

		time_start = jiffies;

		ret = atmel_t6_command(data, MXT_COMMAND_DIAGNOSTIC, page_cmd, 0);
		if (ret) {
			TS_LOG_ERR
			    ("T6 Command DIAGNOSTIC page %d to page %d failed %d!\n",
			     current_page, page, ret);
			return -EIO;
		}

		if (!interval_c)
			return -EAGAIN;

		/*fix me: here need wait every cycle?*/
		timeout_counter = 0;
		do {
			reg = data->T37_address;
			ret = __atmel_read_reg(data, reg,
					     sizeof(command_register),
					     &command_register[0]);
			if (ret) {
				TS_LOG_ERR("T37 read offset 0 failed %d!\n",
					   ret);
				return -EIO;
			}
			if ((u8) command_register[0] != cmd) {
				break;
			}
			if (current_page == command_register[1])
				break;

			msleep(interval);
		} while (timeout_counter++ <= interval_c);

		if (timeout_counter > interval_c) {
			if (retry++ > interval_c) {
				TS_LOG_ERR
				    ("T37 wait cmd %d page %d current page %d timeout, T37 buf(%d %d) page_cmd %d retry %d ##2\n",
				     cmd, page, current_page,
				     (u8) command_register[0],
				     command_register[1], page_cmd, retry);
				return -EBUSY;
			}
		}
		if ((u8) command_register[0] != cmd) {
			TS_LOG_ERR
			    ("T37 wait cmd %d page %d current page %d timeout, T37 buf(%d %d) ##3\n",
			     cmd, page, current_page, (u8) command_register[0],
			     command_register[1]);
			return -EBUSY;
		}
		current_page = command_register[1];

		TS_LOG_ERR("T37 page command ticks %lu\n",
			   jiffies - time_start);
	}

	if (buf) {
		ret =
		    __atmel_read_reg(data,
				   data->T37_address +
				   sizeof(command_register) + index, num, buf);
		if (ret) {
			TS_LOG_ERR
			    ("Failed to read T37 val at page %d.%d (%d)\n",
			     page, index, ret);
			return -EIO;
		}
		/*check the command again*/
		ret =
		    __atmel_read_reg(data, reg, sizeof(command_register),
				   &command_register[0]);
		if (ret) {
			TS_LOG_ERR("T37 read offset 0 failed %d!\n", ret);
			return -EIO;
		}

		if ((u8) command_register[0] != cmd
		    || command_register[1] != page) {
			TS_LOG_ERR("%s, T37 page changed (%d,%d) -> (%d,%d)\n",
				   __func__, cmd, page,
				   (u8) command_register[0],
				   command_register[1]);
			return -EIO;
		}
		//TS_LOG_DEBUG("%s, T37 cmd %d page %d\n", __func__, command_register[0], command_register[1]);
	}

	return 0;
}

extern bool atmel_update_sd_mode;

/*
 *Function for getting t68 product data store.
 *
 *the pds data is read out through t37.
 *
 */
int atmel_pds_get_data(struct mxt_data *data, struct pds_code *code)
{
	int ret = -EINVAL;

	int i = 0;
	int retry = RETRY_TEST_TIME;

	if (!code ||! data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return ret;
	}

#ifdef ROI
	atmel_gMxtT6Cmd58Flag = MXT_T6_CMD58_OFF;
#endif
	do {
		ret =
		    atmel_diagnostic_command(data, MXT_T6_DEBUG_PID, 0, 0,
					   MISC_PDS_HEAD_LEN,
					   (char *)&code->head[0], 10, 5); //10: how long to read once, 5: retry times
		if (ret == 0) {
			TS_LOG_DEBUG("Found PID head %02x %02x.\n",
				     code->head[0], code->head[1]);
			if (code->head[0] == PID_MAGIC_WORD0
			    && code->head[1] == PID_MAGIC_WORD1) {
				ret =
				    atmel_diagnostic_command(data,
							   MXT_T6_DEBUG_PID, 0,
							   MISC_PDS_HEAD_LEN,
							   MISC_PDS_DATA_LEN,
							   (char *)&code->id[0],
							   10, 5);
				if (ret == 0)
					break;
			}
		}
		msleep(10);//ic need wait
	} while (--retry);
#ifdef ROI
	atmel_gMxtT6Cmd58Flag = MXT_T6_CMD58_ON;
#endif

	if (!retry) {
		ret = -ENODATA;
	}

	if (ret == 0) {   //project id   AAAABBCCC   :AAAA product  , BB  IC num,  CCC   module num
		TS_LOG_INFO("Found PDS Data %c%c%c%c%c%c%c%c%c\n",
			    code->id[0], code->id[1], code->id[2], code->id[3],
			    code->id[4], code->id[5], code->id[6], code->id[7],
			    code->id[8]);
		code->tp_color = code->id[MISC_PDS_DATA_LEN - 1];// pds, byte 11 is tp color
		code->id[MISC_PDS_DATA_LEN - 1] = '\0';//byte 11
		TS_LOG_INFO("%s\n", code->id);
	} else {
		TS_LOG_ERR("Read PDS Data failed %d\n", ret);
		memset(code, 0, sizeof(struct pds_code));
	}

	for (i = 0; i < MISC_PDS_DATA_LEN; i++) {//project id 9bytes
		if (code->id[i] < MXT_MIN_CODEID || code->id[i] > MXT_MAX_CODEID) {  // ASCII
			code->id[i] = 0;
			break;
		}
	}

	if (i == 0) {
		strncpy(code->id, "NULL", MISC_PDS_DATA_LEN);
	}

	return ret;
}

static void mxt_update_crc(struct mxt_data *data, u8 cmd, u8 value)
{
	/* on failure, CRC is set to 0 and config will always be downloaded */
	if(!data) {
		TS_LOG_ERR("%s , param invalid\n", __func__);
		return;
	}
	data->config_crc = 0;

	atmel_t6_command(data, cmd, value, true);

	/* Wait for crc message. On failure, CRC is set to 0 and config will
	 * always be downloaded */
	atmel_process_messages(data, MXT_FW_CHG_TIMEOUT);
}

/*
 * mxt_check_reg_init - download configuration to chip
 *
 * Atmel Raw Config File Format
 *
 * The first four lines of the raw config file contain:
 *  1) Version
 *  2) Chip ID Information (first 7 bytes of device memory)
 *  3) Chip Information Block 24-bit CRC Checksum
 *  4) Chip Configuration 24-bit CRC Checksum
 *
 * The rest of the file consists of one line per object instance:
 *	<TYPE> <INSTANCE> <SIZE> <CONTENTS>
 *
 *	<TYPE> - 2-byte object type as hex
 *	<INSTANCE> - 2-byte object instance number as hex
 *	<SIZE> - 2-byte object size as hex
 *	<CONTENTS> - array of <SIZE> 1-byte hex values
 */
static int mxt_check_reg_init(struct mxt_data *data)
{
	struct mxt_info cfg_info;
	struct mxt_object *object = NULL;
	const struct firmware *cfg = NULL;
	int ret = 0;
	int offset= 0;
	int data_pos = 0;
	int byte_offset = 0;
	int i = 0;
	int cfg_start_ofs = 0;
	u32 info_crc = 0, config_crc = 0, calculated_crc = 0;
	u8 *config_mem = 0;
	size_t config_mem_size = 0;
	unsigned int type = 0, instance = 0, size = 0;
	u8 val = 0;
	u16 reg = 0;
#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
	u8 *object_mem = NULL, *object_offset = NULL;
#endif

	if(!data || !data->info ||!data->atmel_dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (0 == strlen(data->cfg_name)) {
		TS_LOG_INFO("Skipping cfg download\n");
		return 0;
	}

	ret = request_firmware(&cfg, data->cfg_name, &data->atmel_dev->dev);
	if (ret < 0) {
		TS_LOG_ERR("Failure to request config file %s\n",
			   data->cfg_name);
		return 0;
	}

	mxt_update_crc(data, MXT_COMMAND_REPORTALL, 1);

	if (strncmp(cfg->data, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		TS_LOG_ERR("Unrecognised config file\n");
		ret = -EINVAL;
		goto release;
	}

	data_pos = strlen(MXT_CFG_MAGIC);

	/* Load information block and check */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		ret = sscanf(cfg->data + data_pos, "%hhx%n",
			     (unsigned char *)&cfg_info + i, &offset);
		if (ret != 1) {
			TS_LOG_ERR("Bad format\n");
			ret = -EINVAL;
			goto release;
		}

		data_pos += offset;
	}

	if (cfg_info.family_id != data->info->family_id) {
		TS_LOG_ERR("Family ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	if (cfg_info.variant_id != data->info->variant_id) {
		TS_LOG_ERR("Variant ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	/* Read CRCs */
	ret = sscanf(cfg->data + data_pos, "%x%n", &info_crc, &offset);
	if (ret != MXT_GET_ONE_PARA) {
		TS_LOG_ERR("Bad format: failed to parse Info CRC\n");
		ret = -EINVAL;
		goto release;
	}
	data_pos += offset;

	ret = sscanf(cfg->data + data_pos, "%x%n", &config_crc, &offset);
	if (ret != MXT_GET_ONE_PARA) {
		TS_LOG_ERR("Bad format: failed to parse Config CRC\n");
		ret = -EINVAL;
		goto release;
	}
	data_pos += offset;

	TS_LOG_INFO("File configure CRC: 0x%06X, Chip configure CRC: 0x%06X\n",
		    config_crc, data->config_crc);
	/* The Info Block CRC is calculated over mxt_info and the object table
	 * If it does not match then we are trying to load the configuration
	 * from a different chip or firmware version, so the configuration CRC
	 * is invalid anyway. */
	if(atmel_update_sd_mode) { //sd update need not compara CRC
		TS_LOG_INFO("%s, update config by sd mode do not judge version\n", __func__);
	} else {
		if (info_crc == data->info_crc) {
			if (config_crc == 0 || data->config_crc == 0) {
				TS_LOG_DEBUG("CRC zero, attempting to apply config\n");
			} else if (config_crc == data->config_crc) {
				TS_LOG_INFO("Config CRC 0x%06X, IC Config CRC 0x%06X: is same\n",
					     info_crc, data->config_crc);
#if defined(CONFIG_MXT_CAL_TRIGGER_CAL_WHEN_CFG_MATCH)
				atmel_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);
#endif
				ret = 0;
				goto release;// exit cfg update
			} else {
				TS_LOG_DEBUG
				    ("Config CRC 0x%06X: does not match file 0x%06X\n",
				     data->config_crc, config_crc);
			}
		} else {
			TS_LOG_ERR
		    	("Warning: Info CRC error - device=0x%06X file=0x%06X\n",
		    	 data->info_crc, info_crc);
		}
	}

	/* Malloc memory to store configuration */
	cfg_start_ofs = MXT_OBJECT_START
	    + data->info->object_num * sizeof(struct mxt_object)
	    + MXT_INFO_CHECKSUM_SIZE;
	config_mem_size = data->mem_size - cfg_start_ofs;
#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
	config_mem_size <<= 1;
#endif
	TS_LOG_INFO("%s, data->mem_size = %d, config_mem_size= %lu\n", __func__, data->mem_size,config_mem_size);
	config_mem = kzalloc(config_mem_size, GFP_KERNEL);
	if (!config_mem) {
		TS_LOG_ERR("Failed to allocate memory\n");
		ret = -ENOMEM;
		goto release;
	}
#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
	object_mem = config_mem + (config_mem_size >> 1);
#endif
	wake_lock(&data->ts_flash_wake_lock);
	while (data_pos < cfg->size - OBJECT_CFG_HEAD_SIZE) {
		/* Read type, instance, length */
		ret = sscanf(cfg->data + data_pos, "%x %x %x%n",
			     &type, &instance, &size, &offset);
#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
		object_offset = object_mem;
#endif
		if (ret == 0) {
			/* EOF */
			break;
		} else if (ret != 3) {	/*at least get 3 para */
			TS_LOG_ERR("Bad format: failed to parse object\n");
			/*ret = -EINVAL;
			   goto release_mem; */
			break;
		}
		data_pos += offset;

		object = atmel_get_object(data, type);
		if (!object) {
			/* Skip object */
			for (i = 0; i < size; i++) {
				ret = sscanf(cfg->data + data_pos, "%hhx%n",
					     &val, &offset);
				data_pos += offset;
			}
			continue;
		}

		if (size > mxt_obj_size(object)) {
			/* Either we are in fallback mode due to wrong
			 * config or config from a later fw version,
			 * or the file is corrupt or hand-edited */
			TS_LOG_ERR("Discarding %ud byte(s) in T%d\n",
				   size - mxt_obj_size(object), type);
		} else if (mxt_obj_size(object) > size) {
			/* If firmware is upgraded, new bytes may be added to
			 * end of objects. It is generally forward compatible
			 * to zero these bytes - previous behaviour will be
			 * retained. However this does invalidate the CRC and
			 * will force fallback mode until the configuration is
			 * updated. We warn here but do nothing else - the
			 * malloc has zeroed the entire configuration. */
			TS_LOG_ERR("Zeroing %d byte(s) in T%d\n",
				   mxt_obj_size(object) - size, type);
		}

		if (instance >= mxt_obj_instances(object)) {
			TS_LOG_ERR("Object instances exceeded!\n");
			ret = -EINVAL;
			goto release_mem;
		}

		reg = object->start_address + mxt_obj_size(object) * instance;

		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->data + data_pos, "%hhx%n",
				     &val, &offset);
			if (ret != MXT_GET_ONE_PARA) {
				TS_LOG_ERR("Bad format in T%d\n", type);
				ret = -EINVAL;
				goto release_mem;
			}
			data_pos += offset;

			if (i > mxt_obj_size(object))
				continue;

			byte_offset = reg + i - cfg_start_ofs;

			if ((byte_offset >= 0)
			    && (byte_offset <= config_mem_size)) {
				*(config_mem + byte_offset) = val;
#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
				*(object_offset++) = val;
#endif
			} else {
				TS_LOG_ERR("Bad object: reg:%d, T%d, ofs=%d\n",
					   reg, object->type, byte_offset);
				ret = -EINVAL;
				goto release_mem;
			}
		}

#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
		ret = __atmel_write_reg(data, reg, size, object_mem);
		if (ret != 0) {
			TS_LOG_ERR("write object[%d] error\n", object->type);
			goto release_mem;
		}
#endif
	}

	/* calculate crc of the received configs (not the raw config file) */
	if (data->T7_address < cfg_start_ofs) {
		TS_LOG_ERR("Bad T7 address, T7addr = %x, config offset %x\n",
			   data->T7_address, cfg_start_ofs);
		ret = 0;
		goto release_mem;
	}

	calculated_crc = atmel_calculate_crc(config_mem,
					   data->T7_address - cfg_start_ofs,
					   config_mem_size);

	if (config_crc > 0 && (config_crc != calculated_crc))
		TS_LOG_DEBUG
		    ("Config CRC different, calculated=%06X, file=%06X\n",
		     calculated_crc, config_crc);

	/* Write configuration as blocks */
	byte_offset = 0;

#if defined(CONFIG_MXT_UPDATE_BY_OBJECT)
	while (byte_offset < config_mem_size) {
		size = config_mem_size - byte_offset;

		if (size > MXT_MAX_BLOCK_WRITE)
			size = MXT_MAX_BLOCK_WRITE;

		ret = __atmel_write_reg(data,
				      cfg_start_ofs + byte_offset,
				      size, config_mem + byte_offset);
		if (ret != 0) {
			TS_LOG_ERR("Config write error, ret=%d\n", ret);
			goto release_mem;
		}

		byte_offset += size;
	}
#endif

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	ret = atmel_soft_reset(data);
	if(ret) {
		TS_LOG_ERR("%s, atmel_soft_reset failed\n", __func__);
		goto release_mem;
	}

	TS_LOG_INFO("Config written\n");

	/* T7 config may have changed */
	ret = atmel_init_t7_power_cfg(data);
	if(ret) {
		TS_LOG_ERR("%s, atmel_init_t7_power_cfg fail\n", __func__);
	}

release_mem:
	wake_unlock(&data->ts_flash_wake_lock);
	kfree(config_mem);
release:
	release_firmware(cfg);
	return ret;
}

int atmel_update_cfg(struct mxt_data *data)
{
	int ret = 0;
	TS_LOG_INFO("atmel_update_cfg\n");

	if (!data)
		return -ENODEV;

	if (data->in_bootloader) {
		TS_LOG_ERR("Not in appmode\n");
		return -EINVAL;
	}

	if (!data->object_table) {
		TS_LOG_ERR("Not initialized\n");
		return -EINVAL;
	}

	atmel_update_config_file_name(data, data->cfg_name);

	data->enable_reporting = false;
	/*there wait for a while after report disable*/

	atmel_free_input_device(data);

	mxt_check_reg_init(data);

	atmel_configure_objects(data);

	/*mxt_acquire_irq(data);*/

	return ret;
}

/*
 * atmel_check_firmware_version - check if firmware need updating.
 *
 * File firmware version is read out from CONFIG file, and then it is compared to the current firmware version.
 */
int atmel_check_firmware_version(struct mxt_data *data)
{
	int error = 0;
	const struct firmware *cfg = NULL;
	int data_pos = 0, offset = 0;
	int i = 0;
	struct mxt_info cfg_info;

	if(!data || !data->atmel_dev) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	if (data->info == NULL) {
		TS_LOG_ERR("%s:data->info = %ld\n", __func__,
			   PTR_ERR(data->info));
		return 0;
	}
	memset(&cfg_info, 0, sizeof(struct mxt_info));
	atmel_update_config_file_name(data, data->cfg_name);

	error = request_firmware(&cfg, data->cfg_name, &data->atmel_dev->dev);
	if (error < 0) {
		TS_LOG_ERR("Fail to get config file %s\n", data->cfg_name);
		return -EINVAL;
	}

	if (strncmp(cfg->data, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		TS_LOG_ERR("Unrecognised config file\n");
		error = -EINVAL;
		goto release;
	}
	data_pos = strlen(MXT_CFG_MAGIC);
	/* Load information block and check */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		error =
		    sscanf(cfg->data + data_pos, "%hhx%n",
			   (unsigned char *)&cfg_info + i, &offset);
		if (error != 1) {
			TS_LOG_ERR("Bad format\n");
			error = -EINVAL;
			goto release;
		}
		data_pos += offset;
	}
	TS_LOG_INFO("File firmware version: 0x%02X.0x%02X\n", cfg_info.version,
		    cfg_info.build);
	TS_LOG_INFO("Chip firmware version: 0x%02X.0x%02X\n", data->info->version,
		    data->info->build);
	if(atmel_update_sd_mode) { //sd update need not to compare version
		error = 0;
		TS_LOG_INFO("%s, update fimware by sd mode do not judge verison\n", __func__);
		goto release;
	}
	/*family_id, variant_id maxtouch chiptype */
	if (cfg_info.family_id != data->info->family_id
	    || cfg_info.variant_id != data->info->variant_id) {
		TS_LOG_INFO
		    ("Firmware is not for this maxtouch chip. Will try update from %d.%d to %d.%d\n",
		     data->info->family_id, data->info->variant_id,
		     cfg_info.family_id, cfg_info.variant_id);
		error = 0;
		goto release;
	}

	/*version,build firmware version */
	if (cfg_info.version != data->info->version
	    || cfg_info.build != data->info->build) {
		TS_LOG_INFO("Firmware need updating from %d.%d to %d.%d\n",
			    data->info->version, data->info->build,
			    cfg_info.version, cfg_info.build);
		error = 0;
		goto release;
	}

	TS_LOG_INFO
	    ("Firmware does not need updating, firmware version and chiptype is the same:family_id:%d, variant_id:%d, version:%d, build:%d\n",
	     data->info->family_id, data->info->variant_id, data->info->version,
	     data->info->build);
	error = -EINVAL;

release:
	release_firmware(cfg);
	return error;
}

int atmel_update_fw_file_name(struct mxt_data *data, char *file_name)
{
	int offset = 0;
	int ret = NO_ERR;
	uint32_t file_name_len = 0;
	if(data == NULL || file_name == NULL || !data->atmel_chip_data
		|| !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	data->is_firmware_broken = false;
	if(strlen(data->module_id) == 0) {
		strncpy(data->module_id, "NULL", strlen("NULL") + 1);
		data->is_firmware_broken = true;
		TS_LOG_ERR("%s, module_id invalid, use default value:NULL\n", __func__);
	}

	if (!atmel_update_sd_mode) {//auto update firmware
		/* Find file by product name. */
		offset = snprintf(file_name, MAX_FILE_NAME_LENGTH - 1, "ts/");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->atmel_chip_data->ts_platform_data->product_name);
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     "_");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->atmel_chip_data->chip_name);
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     "_");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->module_id);
		if(data->fw_need_depend_on_lcd && !data->is_firmware_broken) {
			ret = atmel_get_lcd_panel_info();
			if(ret < 0){
				TS_LOG_ERR("get lcd panel info faild\n");
			}
			ret = atmel_get_lcd_module_name();
			if(ret < 0){
				TS_LOG_ERR("get_lcd_module name failed\n");
			} else {
				offset +=
					snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
						"_");
				offset +=
					snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
						data->lcd_module_name);
			}
		}
		snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			 "_firmware.fw");
	} else {//sd update
		/*
		 * Relative path to /vendor/firmware/
		 * (/vendor is a symbolic link to /system/vendor/)
		 */
		offset =
		    snprintf(file_name, PAGE_SIZE,
			     "ts/atmel_firmware.fw");
	}

	file_name_len = strlen(file_name);
	if (file_name_len < sizeof(data->fw_name)) {
		strncpy(data->fw_name, file_name, file_name_len + 1);
	} else {
		strncpy(data->fw_name, file_name, sizeof(data->fw_name) - 1);
		data->fw_name[sizeof(data->fw_name) - 1] = '\0';
	}

	TS_LOG_INFO("update fw file name: %s\n", data->fw_name);

	return 0;
}

void atmel_update_config_file_name(struct mxt_data *data, char *file_name)
{
	int offset = 0;
	if(!data || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data
		|| !file_name) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return;
	}

	if (!atmel_update_sd_mode) {//auto update
		/* Find file by product name. */
		offset = snprintf(file_name, MAX_FILE_NAME_LENGTH - 1, "ts/");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->atmel_chip_data->ts_platform_data->product_name);
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     "_");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->atmel_chip_data->chip_name);
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     "_");
		offset +=
		    snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
			     data->module_id);
		if(data->fw_need_depend_on_lcd && !data->is_firmware_broken) {//if firmware is broken, use default config name
			offset +=
				snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
					"_");
			offset +=
				snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset,
					data->lcd_module_name);
		}
		snprintf(file_name + offset, MAX_FILE_NAME_LENGTH - 1 - offset, "_config.raw");
	} else {//sd update
		offset =
		    snprintf(file_name, PAGE_SIZE,
			     "ts/atmel_config.raw");
	}

	TS_LOG_INFO("update config file name: %s\n", file_name);
}

static int mxt_check_firmware_format(struct mxt_data *data,
				     const struct firmware *fw)
{
	unsigned int pos = 0;
	char c = '\0';
	if(!data || !fw) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	while (pos < fw->size) {
		c = *(fw->data + pos);

		if (c < '0' || (c > '9' && c < 'A') || c > 'F')
			return 0;

		pos++;
	}

	/* To convert file try
	 * xxd -r -p mXTXXX__APP_VX-X-XX.enc > maxtouch.fw */
	TS_LOG_ERR("Aborting: firmware file must be in binary format\n");

	return -1;
}

static u8 mxt_get_bootloader_version(struct mxt_data *data, u8 val)
{

	u8 buf[3] = {0};	/* 3, read buff len */
	if(!data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}

	if (val & MXT_BOOT_EXTENDED_ID) {
		if (atmel_bootloader_read(data, &buf[0], 3) != 0) {	/* read 3 byte */
			TS_LOG_ERR("%s: i2c failure\n", __func__);
			return -EIO;
		}

		TS_LOG_INFO("Bootloader ID:%d Version:%d\n", buf[1], buf[2]);

		return buf[0];
	} else {
		TS_LOG_INFO("Bootloader ID:%d\n", val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_wait_for_int_status_change(struct mxt_data *data, int timeout)
{
	int irq_gpio = 0;
	if(!data || (timeout < 0) || !data->atmel_chip_data || !data->atmel_chip_data->ts_platform_data) {
		TS_LOG_ERR("%s, param invalid\n", __func__);
		return -EINVAL;
	}
	irq_gpio = data->atmel_chip_data->ts_platform_data->irq_gpio;

	do {
		if (0 == gpio_get_value(irq_gpio)) {
			return 0;
		} else {
			udelay(100);//ic need
		}
	} while (timeout--);

	TS_LOG_INFO("mxt_wait_for_int_status_change timeout\n");
	return -1;
}

static int mxt_check_bootloader(struct mxt_data *data, unsigned int state,
				bool wait)
{
	u8 val = 0;
	int ret = 0;
	if(!data) {
		TS_LOG_ERR("%s ,param invalid\n", __func__);
		return -EINVAL;
	}
recheck:
	if (wait) {
		/*
		 * In application update mode, the interrupt
		 * line signals state transitions. We must wait for the
		 * CHG assertion before reading the status byte.
		 * Once the status byte has been read, the line is deasserted.
		 */
		ret = mxt_wait_for_int_status_change(data, MXT_FW_CHG_TIMEOUT);
		if (ret) {
			/*
			 * TODO: handle -EINTR better by terminating fw update
			 * process before returning to userspace by writing
			 * length 0x000 to device (iff we are in
			 * WAITING_FRAME_DATA state).
			 */
			TS_LOG_ERR("Update wait error %d\n", ret);
			/*don't return false if there is interrupt issue*/
			/*return ret;*/
		}
	}

	ret = atmel_bootloader_read(data, &val, 1);
	if (ret) {
		TS_LOG_ERR("%s:atmel_bootloader_read read failed\n", __func__);
		return ret;
	}

	if (state == MXT_WAITING_BOOTLOAD_CMD)
		val = mxt_get_bootloader_version(data, val);

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK) {
			goto recheck;
		} else if (val == MXT_FRAME_CRC_FAIL) {
			TS_LOG_ERR("Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		TS_LOG_ERR("%s:default branch called\n", __func__);
		return -EINVAL;
	}

	if (val != state) {
		TS_LOG_ERR("Invalid bootloader state %02X != %02X\n",
			   val, state);
		return -EINVAL;
	}

	return 0;
}

static void wait_exit_bootloader(struct mxt_data *data, int timeout)
{
	int ret = 0;
	if(!data || timeout < 0) {
		TS_LOG_ERR("%s ,param invalid\n", __func__);
		return;
	}
	do {
		ret = atmel_probe_info_block(data);
		if (!ret){
			return;
		} else {
			udelay(200);//IC need delay 200us
		}
	} while (timeout--);

	TS_LOG_INFO("wait_exit_bootloader timeout\n");
}

int atmel_load_fw(struct mxt_data *data)
{
	const struct firmware *fw = NULL;
	unsigned int frame_size = 0;
	unsigned int pos = 0;
	unsigned int retry = 0;
	unsigned int frame = 0;
	int ret = 0;

	if(!data) {
		TS_LOG_ERR("%s ,param invalid\n", __func__);
		return -EINVAL;
	}
	if (0 == strlen(data->fw_name)) {
		TS_LOG_ERR("%s:data->fw_name is null\n", __func__);
		return -EEXIST;
	}
	TS_LOG_INFO("atmel_load_fw %s\n", data->fw_name);

	ret = request_firmware(&fw, data->fw_name, &data->atmel_dev->dev);
	if (ret) {
		TS_LOG_ERR("Unable to open firmware %s\n", data->fw_name);
		return ret;
	}

	/* Check for incorrect enc file */
	ret = mxt_check_firmware_format(data, fw);
	if (ret) {
		TS_LOG_ERR("%s: mxt_check_firmware_format fail\n", __func__);
		goto release_firmware;
	}

	if (data->in_bootloader) {
		ret =
		    mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD, false);
		if (ret) {
			TS_LOG_ERR("false bootloader check %d\n", ret);
			data->in_bootloader = false;
		}
	}

	if (!data->in_bootloader) {
		/* Change to the bootloader mode */
		data->in_bootloader = true;

		ret = atmel_t6_command(data, MXT_COMMAND_RESET,
				     MXT_BOOT_VALUE, false);
		if (ret) {
			TS_LOG_ERR("reset to boot loader mode return %d\n",
				   ret);
			/*don't return failed, maybe it's in bootloader mode*/
			/*goto release_firmware;*/
		}
		msleep(200);// ic need 200ms to enter bootloader mode

		atmel_lookup_bootloader_address(data, 0);

		/* At this stage, do not need to scan since we know
		 * family ID */
		do {
			ret =
			    mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD,
						 false);
			if (ret == 0)
				break;
			TS_LOG_ERR("atmel_bootloader_read failed %d retry %d\n",
				   ret, retry);
			atmel_lookup_bootloader_address(data, retry);
		} while (++retry <= RETRY_TEST_TIME);//3:retry times

		if (ret) {
			data->in_bootloader = false;
			goto release_firmware;
		}
	}

	atmel_free_object_table(data);

	ret = mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD, false);
	if (ret) {
		/* Bootloader may still be unlocked from previous update
		 * attempt */
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, false);
		if (ret) {
			TS_LOG_INFO("%s: mxt_check_bootloader fail\n",
				    __func__);
			data->in_bootloader = false;
			goto release_firmware;
		}
	} else {
		TS_LOG_INFO("Unlocking bootloader\n");

		/* Unlock bootloader */
		ret = atmel_send_bootloader_cmd(data, true);
		if (ret) {
			TS_LOG_INFO("atmel_send_bootloader_cmd fail\n");
			data->in_bootloader = false;
			goto release_firmware;
		}
	}
	wake_lock(&data->ts_flash_wake_lock);
	while (pos < fw->size - 1) {
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, true);
		if (ret) {
			TS_LOG_ERR("mxt_check_bootloader fail\n");
			goto release_lock;
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));
		/* Take account of CRC bytes */
		frame_size += 2;

		if ((pos + frame_size) > fw->size || frame_size > MXT_MAX_FRAMESIZE) {
			TS_LOG_ERR("frme_size invalid\n");
			goto release_lock;
		}
		/* Write one frame to device */
		ret = atmel_bootloader_write(data, fw->data + pos, frame_size);
		/*print_hex_dump(KERN_INFO, "[mxt] ", DUMP_PREFIX_OFFSET, 16, 1, fw->data+pos, frame_size, false);*/
		if (ret) {
			TS_LOG_ERR("atmel_bootloader_write fail\n");
			goto release_lock;
		}

		ret = mxt_check_bootloader(data, MXT_FRAME_CRC_PASS, true);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {//retry times
				TS_LOG_ERR("Retry count exceeded\n");
				goto release_lock;
			}
		} else {
			retry = 0;
			pos += frame_size;
			frame++;
		}

		if (frame % 50 == 0)///per 50 frame print info
			TS_LOG_INFO("Sent %d frames, %d/%zd bytes\n",
				     frame, pos, fw->size);
	}

	TS_LOG_INFO("Sent %ud frames, %ud bytes\n", frame, pos);
	mdelay(105);///ic need wait 105ms
	wait_exit_bootloader(data, MXT_FW_RESET_TIME);

	data->in_bootloader = false;
release_lock:
	wake_unlock(&data->ts_flash_wake_lock);
release_firmware:
	release_firmware(fw);
	return ret;
}
