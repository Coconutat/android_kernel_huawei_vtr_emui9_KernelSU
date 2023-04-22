/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2014 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_ic.h"

extern struct himax_ts_data *g_himax_ts_data;
extern int himax_input_config(struct input_dev * input_dev);

int self_test_inter_flag = 0;
void (*himax_ts_flash_work_func)(struct work_struct *work);

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#define FW_SHOW 1
#define FW_DUMP 2
#define FL_SHOW 3
#define FL_DUMP 4

#define FW_COL 32
#define FW_ROW 8

#ifdef HX_TP_SYS_DIAG
	static int 	touch_monitor_stop_limit = 5;
	static uint8_t x_channel = 0;
	static uint8_t y_channel = 0;
	static int diag_command = 0;
	uint8_t diag_self[100] = {0};
	static uint8_t *diag_mutual = NULL;
#endif

#ifdef HX_TP_SYS_REGISTER
	static uint8_t register_command = 0;
	static uint8_t multi_register_command = 0;
	static uint8_t multi_register[8] = {0x00};
	static uint8_t multi_cfg_bank[8] = {0x00};
	static uint8_t multi_value[1024] = {0x00};
	static bool config_bank_reg = false;
#endif

#ifdef HX_TP_SYS_DEBUG
	static int handshaking_result = 0;
	static bool fw_update_complete = false;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[64*1024] = {0};
#endif

#ifdef HX_TP_SYS_FLASH_DUMP
	static uint8_t *flash_buffer = NULL;
	static uint8_t flash_command = 0;//command 0 1 2 3 4
	static uint8_t flash_read_step = 0;
	static uint8_t flash_progress = 0;
	static uint8_t flash_dump_complete = 0;
	static uint8_t flash_dump_fail = 0;
	static uint8_t sys_operation = 0;
	static uint8_t flash_dump_sector = 0;
	static uint8_t flash_dump_page = 0;
	static bool    flash_dump_going = false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);
	static uint8_t getFlashDumpSector(void);
	static uint8_t getFlashDumpPage(void);

	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);

	static void setFlashDumpSector(uint8_t sector);
	static void setFlashDumpPage(uint8_t page);
	static void setFlashDumpGoing(bool going);
#endif

#endif


//=============================================================================================================
//
//	Segment : Himax SYS Debug Function
//
//=============================================================================================================
#ifdef HX_TP_SYS_RESET
static ssize_t himax_reset_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '1')
		himax_HW_reset(true,false);
	return count;
}

static DEVICE_ATTR(reset, (S_IWUSR|S_IRUGO),NULL, himax_reset_set);
#endif
#ifdef HX_TP_SYS_DIAG
uint8_t *getMutualBuffer(void)
{
	return diag_mutual;
}
uint8_t *getSelfBuffer(void)
{
	return &diag_self[0];
}
uint8_t getXChannel(void)
{
	return x_channel;
}
uint8_t getYChannel(void)
{
	return y_channel;
}
uint8_t getDiagCommand(void)
{
	return diag_command;
}
void setDiagCommand(uint8_t cmd)
{
	diag_command = cmd;
}
void setXChannel(uint8_t x)
{
	x_channel = x;
}
void setYChannel(uint8_t y)
{
	y_channel = y;
}
void setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(uint8_t), GFP_KERNEL);
	if (diag_mutual == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
	}
}
void freeMutualBuffer(void)
{
	if (diag_mutual)
		kfree(diag_mutual);
	diag_mutual = NULL;
}
static ssize_t himax_diag_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int count = 0;
	uint32_t loop_i =0;
	uint16_t width = 0;
	uint16_t self_num =0;
	uint16_t mutual_num=0;
	int m = 0;
	width		= x_channel;
	self_num		= x_channel + y_channel;
	mutual_num	= x_channel * y_channel;
	count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"ChannelStart: %4d, %4d\n\n", x_channel, y_channel);
	// start to show out the raw data in adb shell
	if (diag_command >= 1 && diag_command <= 6) {
		if (diag_command <= 3) {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,"%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
				{
					m=width + loop_i/width;
					count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count, " %3d\n", diag_self[m]);
				}
			}

			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,  "\n");
			for (loop_i = 0; loop_i < width; loop_i++) {
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
					count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
			}
		} else if (diag_command > 4) {
			for (loop_i = 0; loop_i < self_num; loop_i++) {
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%4d", diag_self[loop_i]);
				if (((loop_i - mutual_num) % width) == (width - 1))
					count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
			}
		} else {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
					count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
			}
		}
		count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "ChannelEnd");
		count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
	} else if (diag_command == 7) {
		for (loop_i = 0; loop_i < 128 ;loop_i++) {
			if ((loop_i % 16) == 0) {
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "LineStart:");
			}
			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%4d", diag_coor[loop_i]);

			if ((loop_i % 16) == 15)
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
		}
	}
	return count;
}

static ssize_t himax_diag_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	const uint8_t command_ec_128_raw_flag 		= 0x02;
	const uint8_t command_ec_24_normal_flag 	= 0x00;
	uint8_t command_ec_128_raw_baseline_flag 	= 0x01;
	uint8_t command_ec_128_raw_bank_flag 		= 0x03;
	uint8_t command_F1h[2] = {HX_REG_RAWDATA_MODE, 0x00};
	int	retval =	0;
	if (count >= 80)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	diag_command = buf[0] - '0';

	TS_LOG_INFO("[Himax]diag_command=0x%x\n",diag_command);
	if (diag_command == 0x01)	{//DC
		command_F1h[1] = command_ec_128_raw_baseline_flag;
		TS_LOG_ERR("[Himax]diag_command=0x%x, data = %d\n",command_F1h[0], command_F1h[1]);
		retval = i2c_himax_write(command_F1h[0] ,&command_F1h[1], 1, sizeof(command_F1h), DEFAULT_RETRY_CNT);
		if(retval<0) {
			return -EFAULT;
		}
	} else if (diag_command == 0x02) {//IIR
		command_F1h[1] = command_ec_128_raw_flag;
		retval =i2c_himax_write(command_F1h[0] ,&command_F1h[1], 1, sizeof(command_F1h), DEFAULT_RETRY_CNT);
		if(retval<0) {
			return -EFAULT;
		}
	} else if (diag_command == 0x03) {	//BANK
		command_F1h[1] = command_ec_128_raw_bank_flag;	//0x03
		retval =i2c_himax_write(command_F1h[0] ,&command_F1h[1], 1, sizeof(command_F1h), DEFAULT_RETRY_CNT);
		if(retval<0) {
			return -EFAULT;
		}
	} else if (diag_command == 0x04 ) { // 2T3R IIR
		command_F1h[1] = 0x04; //2T3R IIR
		retval =i2c_himax_write(command_F1h[0] ,&command_F1h[1], 1, sizeof(command_F1h), DEFAULT_RETRY_CNT);
		if(retval<0) {
			return -EFAULT;
		}
	} else if (diag_command == 0x00) { //Disable
		command_F1h[1] = command_ec_24_normal_flag;
		retval =i2c_himax_write(command_F1h[0] ,&command_F1h[1], 1, sizeof(command_F1h), DEFAULT_RETRY_CNT);
		if(retval<0) {
			return -EFAULT;
		}
		touch_monitor_stop_flag = touch_monitor_stop_limit;
	}
	else{
		TS_LOG_ERR("[Himax]Diag command error!diag_command=0x%x\n",diag_command);
	}
	return count;
}
static DEVICE_ATTR(diag, (S_IWUSR|S_IRUGO),himax_diag_show, himax_diag_dump); //Debug_diag_done

#endif

#ifdef HX_TP_SYS_REGISTER
static ssize_t himax_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int retval = 0;
	int base = 0;
	uint16_t loop_i  = 0;
	uint16_t loop_j  = 0;
	uint8_t inData[128] = {0};
	uint8_t outData[5] = {0};
	TS_LOG_INFO("Himax multi_register_command = %d \n",multi_register_command);
	if (multi_register_command == 1) {
		base = 0;

		for(loop_i = 0; loop_i < 6; loop_i++) {
			if (multi_register[loop_i] != 0x00) {
				if (multi_cfg_bank[loop_i] == 1) {//config bank register
					if(IC_TYPE == HX_85XX_F_SERIES_PWON) {
							outData[0] = HX_REG_SRAM_TEST_MODE_EN;
							} else {
									outData[0] = 0x14;
					}
					retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
					if(retval < 0) {
						return retval;
					}
					msleep(HX_SLEEP_10MS);
					/*write sram start addr*/
					outData[0] = 0x00;
					outData[1] = multi_register[loop_i];
					retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&outData[0], 2, sizeof(outData), DEFAULT_RETRY_CNT);
					if(retval < 0) {
						return retval;
					}
					msleep(HX_SLEEP_10MS);

					retval = i2c_himax_read(HX_REG_FLASH_RPLACE, inData, 128, sizeof(inData), DEFAULT_RETRY_CNT);
					if(retval < 0) {
						return retval;
					}
					outData[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
					retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
					if(retval < 0) {
						return retval;
					}
					for(loop_j=0; loop_j<128; loop_j++)
						multi_value[base++] = inData[loop_j];
				} else {//normal register
					retval = i2c_himax_read( multi_register[loop_i], inData, 128, sizeof(inData), DEFAULT_RETRY_CNT);
					if(retval < 0) {
						return retval;
					}
					for(loop_j=0; loop_j<128; loop_j++)
						multi_value[base++] = inData[loop_j];
				}
			}
		}

		base = 0;
		retval=0;
		for(loop_i = 0; loop_i < 6; loop_i++) {
			if (multi_register[loop_i] != 0x00) {
				if (multi_cfg_bank[loop_i] == 1)
					retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Register: FE(%x)\n", multi_register[loop_i]);
				else
					retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Register: %x\n", multi_register[loop_i]);

				for (loop_j = 0; loop_j < 128; loop_j++) {
					retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X ", multi_value[base++]);
					if ((loop_j % 16) == 15)
						retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
				}
			}
		}
		return retval;
		}

		if (config_bank_reg) {
			TS_LOG_INFO("%s: register_command = FE(%x)\n", __func__, register_command);

			//Config bank register read flow.
			if(IC_TYPE == HX_85XX_F_SERIES_PWON) {
				outData[0] = HX_REG_SRAM_TEST_MODE_EN;
				} else {
					outData[0] = 0x14;
				}
		retval = i2c_himax_write(HX_REG_SRAM_SWITCH,&outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
		if(retval < 0) {
			return retval;
		}
		msleep(HX_SLEEP_10MS);
		/*write sram start addr*/
		outData[0] = 0x00;
		outData[1] = register_command;
		retval = i2c_himax_write(HX_REG_SRAM_ADDR,&outData[0], 2, sizeof(outData), DEFAULT_RETRY_CNT);
		if(retval < 0) {
			return retval;
		}
		msleep(HX_SLEEP_10MS);

		retval = i2c_himax_read(HX_REG_FLASH_RPLACE, inData, 128, sizeof(inData), DEFAULT_RETRY_CNT);
		if(retval < 0) {
			return retval;
		}
		msleep(HX_SLEEP_10MS);

		outData[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
		retval = i2c_himax_write( HX_REG_SRAM_SWITCH,&outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
		if(retval < 0) {
			return retval;
		}
		} else {
			if (i2c_himax_read( register_command, inData, 128, sizeof(inData), DEFAULT_RETRY_CNT) < 0)
				return retval;
		}

		if (config_bank_reg)
			retval += sprintf(buf, "command: FE(%x)\n", register_command);
		else
			retval += sprintf(buf, "command: %x\n", register_command);

		for (loop_i = 0; loop_i < HX_RECEIVE_BUF_MAX_SIZE; loop_i++) {
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X ", inData[loop_i]);
			if ((loop_i % 16) == 15)
				retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		}
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");

	return retval;
}

static ssize_t himax_register_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6]={0}, length = 0;
	unsigned long result    = 0;
	uint8_t loop_i          = 0;
	uint16_t base           = 5;
	uint8_t write_da[HX_RECEIVE_BUF_MAX_SIZE]={0};
	uint8_t outData[5]={0};
	int retval = 0;
	if (count >= 80)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(write_da, 0x0, sizeof(write_da));
	memset(outData, 0x0, sizeof(outData));

	TS_LOG_INFO("himax %s \n",buf);

	if (buf[0] == 'm' && buf[1] == 'r' && buf[2] == ':') {
		memset(multi_register, 0x00, sizeof(multi_register));
		memset(multi_cfg_bank, 0x00, sizeof(multi_cfg_bank));
		memset(multi_value, 0x00, sizeof(multi_value));

		TS_LOG_INFO("himax multi register enter\n");

		multi_register_command = 1;

		base 	= 2;
		loop_i 	= 0;
		while(true) {
			if (buf[base] == '\n')
				break;
			if (loop_i >= 6 )
				break;
			if (buf[base] == ':' && buf[base+1] == 'x' && buf[base+2] == 'F' &&
					buf[base+3] == 'E' && buf[base+4] != ':') {
				memcpy(buf_tmp, buf + base + 4, 2);
				if (!strict_strtoul(buf_tmp, 16, &result)) {
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 1;
				}
				base += 6;
			} else {
				memcpy(buf_tmp, buf + base + 2, 2);
				if (!strict_strtoul(buf_tmp, 16, &result)) {
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 0;
				}
				base += 4;
			}
		}

		for(loop_i = 0; loop_i < 6; loop_i++)
			TS_LOG_INFO("%d,%d:",multi_register[loop_i],multi_cfg_bank[loop_i]);
		TS_LOG_INFO("\n");
	} else if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':') {
		multi_register_command = 0;

		if (buf[2] == 'x') {
			if (buf[3] == 'F' && buf[4] == 'E') {//Config bank register
				config_bank_reg = true;
				memcpy(buf_tmp, buf + 5, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
					register_command = result;
				base = 7;
				TS_LOG_INFO("CMD: FE(%x)\n", register_command);
			} else {
				config_bank_reg = false;
				memcpy(buf_tmp, buf + 3, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
					register_command = result;
				base = 5;
				TS_LOG_INFO("CMD: %x\n", register_command);
			}
			for (loop_i = 0; loop_i < HX_RECEIVE_BUF_MAX_SIZE; loop_i++) {
				if (buf[base] == '\n') {
					if (buf[0] == 'w') {
						if (config_bank_reg) {
							if(IC_TYPE == HX_85XX_F_SERIES_PWON)
							{
								outData[0] = HX_REG_SRAM_TEST_MODE_EN;
							}
							else
							{
								outData[0] = HX_REG_E_SRAM_TEST_MODE_EN;
							}
								retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
								if(retval < 0) {
									return -EFAULT;
								}
								msleep(HX_SLEEP_10MS);
								outData[0] = 0x00;
								outData[1] = register_command;
								retval = i2c_himax_write(HX_REG_SRAM_ADDR, &outData[0], 2, sizeof(outData), DEFAULT_RETRY_CNT);
								if(retval < 0) {
									return -EFAULT;
								}
								msleep(HX_SLEEP_10MS);
								retval = i2c_himax_write(HX_REG_FLASH_WPLACE, &write_da[0], length, sizeof(write_da), DEFAULT_RETRY_CNT);
								if(retval < 0) {
									return -EFAULT;
								}
								msleep(HX_SLEEP_10MS);
								outData[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
								retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &outData[0], 1, sizeof(outData), DEFAULT_RETRY_CNT);
								if(retval  <0) {
									return -EFAULT;
								}
							TS_LOG_INFO("CMD: FE(%x), %x, %d\n", register_command,write_da[0], length);
						}
						else {
							retval = i2c_himax_write( register_command, &write_da[0], length, sizeof(write_da), DEFAULT_RETRY_CNT);
							if (retval < 0) {
								TS_LOG_ERR("%s: himax i2c write failed\n", __func__);
							}
							TS_LOG_INFO("CMD: %x, %x, %d\n", register_command,write_da[0], length);
						}
					}
					TS_LOG_INFO("\n");
					return count;
				}
				if (buf[base + 1] == 'x') {
					buf_tmp[4] = '\n';
					buf_tmp[5] = '\0';
					memcpy(buf_tmp, buf + base + 2, 2);
					if (!strict_strtoul(buf_tmp, 16, &result)) {
						write_da[loop_i] = result;
					}
					length++;
				}
				base += 4;
			}
		}
	}
	return count;
}

static DEVICE_ATTR(register, (S_IWUSR|S_IRUGO),himax_register_show, himax_register_store); //Debug_register_done

#endif
#ifdef HX_TP_SYS_DEBUG
static uint8_t himax_read_FW_ver(bool hw_reset)
{
	uint8_t cmd[3] = {0};

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);

	if (hw_reset) {
		himax_HW_reset(HX_LOADCONFIG_DISABLE,HX_INT_EN);
	}

	msleep(HX_SLEEP_120MS);
	if (i2c_himax_read( HX_VER_FW_MAJ, cmd, 1, sizeof(cmd), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return I2C_ACCESS_FAIL;
	}
	g_himax_ts_data->vendor_fw_ver_H = cmd[0];
	if (i2c_himax_read(HX_VER_FW_MIN, cmd, 1, sizeof(cmd), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return I2C_ACCESS_FAIL;
	}
	g_himax_ts_data->vendor_fw_ver_L = cmd[0];
	TS_LOG_INFO("FW_VER : %d,%d \n",g_himax_ts_data->vendor_fw_ver_H,g_himax_ts_data->vendor_fw_ver_L);

	if (i2c_himax_read( HX_VER_FW_CFG, cmd, 1, sizeof(cmd), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return I2C_ACCESS_FAIL;
	}
	g_himax_ts_data->vendor_config_ver = cmd[0];
	TS_LOG_INFO("CFG_VER : %d \n",g_himax_ts_data->vendor_config_ver);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_EN);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

	return NO_ERR;
}

static ssize_t himax_debug_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t retval = 0;
	if (debug_level_cmd == 't')
	{
		if (fw_update_complete)
		{
			retval += sprintf(buf, "FW Update Complete ");
		}
		else
		{
			retval += sprintf(buf, "FW Update Fail ");
		}
	}
	else if (debug_level_cmd == 'h')
	{
		if (handshaking_result == 0)
		{
			retval += sprintf(buf, "Handshaking Result = %d (MCU Running)\n",handshaking_result);
		}
		else if (handshaking_result == 1)
		{
			retval += sprintf(buf, "Handshaking Result = %d (MCU Stop)\n",handshaking_result);
		}
		else if (handshaking_result == 2)
		{
			retval += sprintf(buf, "Handshaking Result = %d (I2C Error)\n",handshaking_result);
		}
		else
		{
			retval += sprintf(buf, "Handshaking Result = error \n");
		}
	}
	else if (debug_level_cmd == 'v')
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FW_VER = ");
           retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X, %2.2X \n",g_himax_ts_data->vendor_fw_ver_H,g_himax_ts_data->vendor_fw_ver_L);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "CONFIG_VER = ");
           retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X \n",g_himax_ts_data->vendor_config_ver);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	}
	else if (debug_level_cmd == 'd')
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Himax Touch IC Information :\n");
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : D\n");
		}
		else if (IC_TYPE == HX_85XX_E_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : E\n");
		}
		else if (IC_TYPE == HX_85XX_ES_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : ES\n");
		}
		else if (IC_TYPE == HX_85XX_F_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : F\n");
		}
		else
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type error.\n");
		}

		if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : SW\n");
		}
		else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : HW\n");
		}
		else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : CRC\n");
		}
		else
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum error.\n");
		}

		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Interrupt : LEVEL TRIGGER\n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "RX Num : %d\n",HX_RX_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "TX Num : %d\n",HX_TX_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "BT Num : %d\n",HX_BT_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "X Resolution : %d\n",HX_X_RES);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Y Resolution : %d\n",HX_Y_RES);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Max Point : %d\n",HX_MAX_PT);
	}
	else if (debug_level_cmd == 'i')
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Himax Touch Driver Version:\n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%s \n", HIMAX_DRIVER_VER);
	}
	return retval;
}

static ssize_t himax_debug_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	int result = 0;
	char fileName[HX_RECEIVE_BUF_MAX_SIZE]= {0};
	mm_segment_t oldfs;
	struct file* hx_filp = NULL;
	TS_LOG_INFO("%s: enter\n", __func__);

	if (count >= 80)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if ( buf[0] == 'h') //handshaking
	{
		debug_level_cmd = buf[0];

		himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);

		handshaking_result = himax_hand_shaking(); //0:Running, 1:Stop, 2:I2C Fail

		himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

		return count;
	}

	else if ( buf[0] == 'v') //firmware version
	{
		debug_level_cmd = buf[0];
		himax_read_FW_ver(true);
		return count;
	}

	else if ( buf[0] == 'd') //test
	{
		debug_level_cmd = buf[0];
		return count;
	}

	else if ( buf[0] == 'i') //driver version
	{
		debug_level_cmd = buf[0];
		return count;
	}

	else if (buf[0] == 't')
	{
		himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
#ifdef HX_CHIP_STATUS_MONITOR
		HX_CHIP_POLLING_COUNT = 0;
		cancel_delayed_work_sync(&g_himax_ts_data->himax_chip_monitor);
#endif

		debug_level_cmd 		= buf[0];
		fw_update_complete		= false;

		memset(fileName, 0, sizeof(fileName));
		// parse the file name
		snprintf(fileName, count-2, "%s", &buf[2]);
		TS_LOG_INFO("%s: upgrade from file(%s) start!\n", __func__, fileName);
		// open file
		hx_filp = filp_open(fileName, O_RDONLY, 0);
		if (IS_ERR(hx_filp))
		{
			TS_LOG_ERR("%s: open firmware file failed\n", __func__);
			goto firmware_upgrade_done;
		}
		oldfs = get_fs();
		/*lint -save -e* */
		set_fs(KERNEL_DS);
		/*lint -restore*/
		// read the latest firmware binary file
		result=hx_filp->f_op->read(hx_filp,upgrade_fw,sizeof(upgrade_fw), &hx_filp->f_pos);
		if (result < 0)
		{
			TS_LOG_ERR("%s: read firmware file failed\n", __func__);
			set_fs(oldfs);
			goto firmware_upgrade_done;
		}

		set_fs(oldfs);
		filp_close(hx_filp, NULL);

		TS_LOG_INFO("%s: upgrade start,len %d: %02X, %02X, %02X, %02X\n", __func__, result, upgrade_fw[0], upgrade_fw[1], upgrade_fw[2], upgrade_fw[3]);

		if (result > 0)
		{
			// start to upgrade
			himax_HW_reset(HX_LOADCONFIG_DISABLE,HX_INT_EN);;

			if (fts_ctpm_fw_upgrade_with_fs(upgrade_fw, result, true) == 0)
			{
				TS_LOG_ERR("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
				fw_update_complete = false;
			}
			else
			{
				TS_LOG_INFO("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
				fw_update_complete = true;

				g_himax_ts_data->vendor_fw_ver_H = upgrade_fw[FW_VER_MAJ_FLASH_ADDR];
				g_himax_ts_data->vendor_fw_ver_L = upgrade_fw[FW_VER_MIN_FLASH_ADDR];
				g_himax_ts_data->vendor_config_ver = upgrade_fw[FW_CFG_VER_FLASH_ADDR];
			}
			himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_EN);
			goto firmware_upgrade_done;
		}
	}

	firmware_upgrade_done:

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif

	return count;
}

static DEVICE_ATTR(debug, (S_IWUSR|S_IRUGO),himax_debug_show, himax_debug_dump); //Debug_debug_done
#endif

#ifdef HX_TP_SYS_FLASH_DUMP

static uint8_t getFlashCommand(void)
{
	return flash_command;
}

static uint8_t getFlashDumpProgress(void)
{
	return flash_progress;
}

static uint8_t getFlashDumpComplete(void)
{
	return flash_dump_complete;
}

static uint8_t getFlashDumpFail(void)
{
	return flash_dump_fail;
}

static uint8_t getSysOperation(void)
{
	return sys_operation;
}

static uint8_t getFlashReadStep(void)
{
	return flash_read_step;
}

static uint8_t getFlashDumpSector(void)
{
	return flash_dump_sector;
}

static uint8_t getFlashDumpPage(void)
{
	return flash_dump_page;
}

bool getFlashDumpGoing(void)
{
	return flash_dump_going;
}

void setFlashBuffer(void)
{
	flash_buffer = kzalloc(FLASH_SIZE * sizeof(uint8_t), GFP_KERNEL);
	if (!flash_buffer) {
		TS_LOG_ERR("%s: kzalloc fail\n", __func__);
	}
	memset(flash_buffer,0x00,FLASH_SIZE);
}

void freeFlashBuffer(void)
{
	if (flash_buffer)
		kfree(flash_buffer);
	flash_buffer = NULL;
}

void setSysOperation(uint8_t operation)
{
	sys_operation = operation;
}

static void setFlashDumpProgress(uint8_t progress)
{
	flash_progress = progress;
}

static void setFlashDumpComplete(uint8_t status)
{
	flash_dump_complete = status;
}

static void setFlashDumpFail(uint8_t fail)
{
	flash_dump_fail = fail;
}

static void setFlashCommand(uint8_t command)
{
	flash_command = command;
}

static void setFlashReadStep(uint8_t step)
{
	flash_read_step = step;
}

static void setFlashDumpSector(uint8_t sector)
{
	flash_dump_sector = sector;
}

static void setFlashDumpPage(uint8_t page)
{
	flash_dump_page = page;
}

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

int  himax_ts_flash_work_func_case1(int x44of2,int x44of3,uint8_t *page_tmp)
{
	uint8_t x59_tmp[4] = {0};
	int k = 0;
	int l = 0;
	uint8_t x44_command[4] = {HX_REG_SET_FLASH_ADDR,0x00,0x00,0x00};
	uint8_t x46_command[2] = {HX_REG_FLASH_TRASFER,0x00};
	int m = 0;
	for(k=0; k<32; k++)
	{
		x44_command[1] = k;
		x44_command[2] = x44of2;  //i
		x44_command[3] = x44of3;  //j
		if ( i2c_himax_write(x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		if ( i2c_himax_write_command(x46_command[0], DEFAULT_RETRY_CNT) < 0)
		{
			TS_LOG_ERR("%s i2c write 46 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		if ( i2c_himax_read(0x59, x59_tmp, 4, sizeof(x59_tmp), DEFAULT_RETRY_CNT) < 0)
		{
			TS_LOG_ERR("%s i2c write 59 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		for(l=0; l<4; l++)
		{
			m = k*4+l;
			page_tmp[m] = x59_tmp[l];
		}
	}
	return NO_ERR;
Flash_Dump_i2c_transfer_error:
	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return I2C_WORK_ERR;
}
int  himax_ts_flash_work_func_case2(int page,int sector,uint8_t *page_tmp)
{
	int i = 0;
	int j = 0;
	uint8_t x44_command[4] = {HX_REG_SET_FLASH_ADDR,0x00,0x00,0x00};
	uint8_t x46_command[2] = {HX_REG_FLASH_TRASFER,0x00};
	uint8_t x59_tmp[4] = {0};
	int m = 0;
	for(i=0; i<64; i++)
	{
		x44_command[1] = i;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write( x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		if ( i2c_himax_write_command( x46_command[0], DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 46 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		/*0x59 :flash dump data addr */
		if ( i2c_himax_read(0x59, x59_tmp, 4, sizeof(x59_tmp), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 59 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		for(j=0; j<4; j++)
		{
			m = i*4+j;
			page_tmp[m] = x59_tmp[j];
			//page_tmp[i*4+j] = x59_tmp[j];
		}
	}
	return NO_ERR;
Flash_Dump_i2c_transfer_error:
	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return I2C_WORK_ERR;
}
void himax_ts_flash_work_func_print_buffer(int buffer_ptr)
{
	int i = 0;
	TS_LOG_INFO(" buffer_ptr = %d \n",buffer_ptr);
	for (i = 0; i < buffer_ptr; i++)
	{
		TS_LOG_INFO("%2.2X ", flash_buffer[i]);
		if ((i % 16) == 15)
		{
			TS_LOG_INFO("\n");
		}
	}
	TS_LOG_INFO("%s End\n",__func__);
}
void hx852xf_ts_flash_work_func(struct work_struct *work)
{
	int buffer_ptr = 0;
	int i=0, j=0;
	int retval = 0;
	uint8_t sector = 0;
	uint8_t page = 0;
	uint8_t page_tmp[HX_RECEIVE_BUF_MAX_SIZE]  = {0};
	uint8_t local_flash_command = 0;

	uint8_t x81_command[2] = {HX_CMD_TSSLPOUT,0x00};
	uint8_t x82_command[2] = {HX_CMD_TSSOFF,0x00};
	uint8_t x42_command[2] = {HX_REG_FLASH_MANUAL_MODE,HX_REG_FLASH_MANUAL_OFF};
	uint8_t x43_command[4] = {HX_REG_SET_FLASH_EN,0x00,0x00,0x00};
	uint8_t x44_command[4] = {HX_REG_SET_FLASH_ADDR,0x00,0x00,0x00};
	uint8_t x45_command[5] = {0x45,0x00,0x00,0x00,0x00};
	uint8_t x4D_command[2] = {0x4D,0x00};
	uint8_t x5B_command[2] = {HX_REG_SET_FLASH_MANUAL_0,0x00};
	uint8_t x5C_command[2] = {HX_REG_SET_FLASH_MANUAL_1,0x00};
	int m = 0;
	TS_LOG_INFO("%s: Entring\n",__func__);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	cancel_delayed_work_sync(&g_himax_ts_data->himax_chip_monitor);
#endif

	setFlashDumpGoing(true);

	sector = getFlashDumpSector();
	page = getFlashDumpPage();

	local_flash_command = getFlashCommand();

	if(local_flash_command<0x0F) {
		himax_HW_reset(HX_LOADCONFIG_DISABLE,HX_INT_DISABLE);
	}
	if ( i2c_himax_master_write(x81_command, 1, sizeof(x81_command), DEFAULT_RETRY_CNT) < 0 )//sleep out
	{
		TS_LOG_ERR("%s i2c write x81_command fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(HX_SLEEP_120MS);

	if ( i2c_himax_master_write(x82_command, 1, sizeof(x82_command), DEFAULT_RETRY_CNT) < 0 )
	{
		TS_LOG_ERR("%s i2c write x81_command fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(HX_SLEEP_100MS);

	TS_LOG_INFO("%s: local_flash_command = %d enter.\n", __func__,local_flash_command);

	if ((local_flash_command == FW_SHOW || local_flash_command == FW_DUMP)|| (local_flash_command==0x0F))
	{
		/*set flash dump enable:D0 bit*/
		x43_command[1] = 0x01;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0)
		{
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<64 ; j++)
			{
				memset(page_tmp,0x00,sizeof(page_tmp));
				himax_ts_flash_work_func_case1(i,j,page_tmp);

				TS_LOG_INFO("%s: Run new method\n",__func__);
				memcpy(&flash_buffer[buffer_ptr],page_tmp,HX_RECEIVE_BUF_MAX_SIZE);
				buffer_ptr+=HX_RECEIVE_BUF_MAX_SIZE;
				setFlashDumpProgress(i*32 + j);
			}
		}
	}
	else if (local_flash_command == FL_SHOW)
	{
		/*set flash dump enable*/
		x43_command[1] = 0x01;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		memset(page_tmp,0x00,sizeof(page_tmp));

		himax_ts_flash_work_func_case2(page,sector,page_tmp);

		TS_LOG_INFO("%s: Run new method\n",__func__);
		memcpy(&flash_buffer[buffer_ptr],page_tmp,HX_RECEIVE_BUF_MAX_SIZE);
		buffer_ptr += HX_RECEIVE_BUF_MAX_SIZE;
	}
	else if (local_flash_command == FL_DUMP)
	{
		himax_lock_flash(0);

		msleep(HX_SLEEP_50MS);

		/*page erase*/
		x43_command[1] = 0x01;
		/*set flash manual_0*/
		x43_command[2] = 0x00;
		/*set flash manual_1*/
		x43_command[3] = 0x02;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5C_command[0],&x43_command[3], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5C fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash addr*/
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		if ( i2c_himax_write_command(x4D_command[0], DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		/*enter manual mode*/
		x42_command[1] = 0x01;
		if( i2c_himax_write(x42_command[0],&x42_command[1], 1, sizeof(x42_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		/*set flash enable*/
		x43_command[1] = 0x01;
		/*set flash manual*/
		x43_command[2] = 0x00;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write( x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		/*set flash address*/
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(x44_command[0],&x44_command[1], sizeof(x44_command), 3, DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		/*manual mode command : 47 to latch the flash address when page address change*/
		x43_command[1] = 0x01;
		/*set ctrl flash register*/
		x43_command[2] = 0x09;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write( x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = 0x01;
		/*set ctrl flash register*/
		x43_command[2] = 0x0D;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write( x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = 0x01;
		/*set ctrl flash register*/
		x43_command[2] = 0x09;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		for(i=0; i<64; i++)
		{
			TS_LOG_INFO("himax :i=%d \n",i);
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;
			if ( i2c_himax_write( x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);
			m = i*4;
			x45_command[1] = flash_buffer[m];
			m+=1;
			x45_command[2] = flash_buffer[m];
			m+=1;
			x45_command[3] = flash_buffer[m];
			m+=1;
			x45_command[4] = flash_buffer[m];
			if ( i2c_himax_write( x45_command[0],&x45_command[1], 4, sizeof(x45_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 45 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);

			// manual mode command : 48 ,data will be written into flash buffer
			x43_command[1] = 0x01;
			x43_command[2] = 0x0D;
			if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if ( i2c_himax_write( x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);
			/*set flash enable*/
			x43_command[1] = 0x01;
			x43_command[2] = 0x09;
			if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);
		}

		// manual mode command : 49 ,program data from flash buffer to this page
		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = 0x01;
		x43_command[2] = 0x05;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = 0x01;
		/*set flash manual*/
		x43_command[2] = 0x01;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(x5B_command[0],&x43_command[2], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		// flash disable
		x43_command[1] = 0x00;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		// leave manual mode
		x42_command[1] = 0x01;
		if( i2c_himax_write( x42_command[0],&x42_command[1], 1, sizeof(x42_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(HX_SLEEP_10MS);

		// lock flash
		himax_lock_flash(1);
		msleep(HX_SLEEP_50MS);

		buffer_ptr = HX_RECEIVE_BUF_MAX_SIZE;
		TS_LOG_INFO("Himax: Flash page write Complete\n");
	}

	TS_LOG_INFO("Complete\n");
	if(local_flash_command==FW_SHOW)
		{
		himax_ts_flash_work_func_print_buffer(buffer_ptr);

		}

	retval = i2c_himax_master_write( x43_command, 1, sizeof(x43_command), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("%s: i2c write fail\n", __func__);
	}
	msleep(HX_SLEEP_50MS);

	if (local_flash_command == FW_DUMP)
	{
		struct file *fn;

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,buffer_ptr*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}

	if(local_flash_command<0x0F)
		himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	TS_LOG_INFO("%s: End\n",__func__);
	return;

Flash_Dump_i2c_transfer_error:

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
}
#define HX_REG_FLASH_DUMP_CMD 0x45
#define HX_REG_FLASH_DUMP_TRANS_CMD 0x4D
#define HX_FLASH_MAX_VAL 0x0F
#define HX_DATA_INIT_VAL 0x00
#define X43_COMMAND_PAGE_ERASE 0x00020001
#define HX_FLASH_MANUAL_MODE_EN 0x01
#define HX_FLASH_MANUAL_MODE_DIS 0x01
#define HX_FLASH_CTRL_CMD_5 0x05
#define HX_FLASH_CTRL_CMD_9 0x09
#define HX_FLASH_CTRL_CMD_D 0x0D
void hx852xes_ts_flash_work_func(struct work_struct *work)
{
	int buffer_ptr = 0;
	int i=0, j=0;
	int retval = 0;
	uint8_t sector = 0;
	uint8_t page = 0;
	uint8_t page_tmp[HX_RECEIVE_BUF_MAX_SIZE]  = {0};
	uint8_t local_flash_command = 0;

	uint8_t x81_command[2] = {HX_CMD_TSSLPOUT,0x00};
	uint8_t x82_command[2] = {HX_CMD_TSSOFF,0x00};
	uint8_t x35_command[2] = {HX852XES_REG_FLASH_MANUAL_MODE,HX_REG_FLASH_MANUAL_OFF};
	uint8_t x43_command[4] = {HX_REG_SET_FLASH_EN,0x00,0x00,0x00};
	uint8_t x44_command[4] = {HX_REG_SET_FLASH_ADDR,0x00,0x00,0x00};
	uint8_t x45_command[5] = {HX_REG_FLASH_DUMP_CMD,0x00,0x00,0x00,0x00};
	uint8_t x4D_command[2] = {HX_REG_FLASH_DUMP_TRANS_CMD,0x00};
	int m = 0;
	TS_LOG_INFO("%s: Entring\n",__func__);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	cancel_delayed_work_sync(&g_himax_ts_data->himax_chip_monitor);
#endif

	setFlashDumpGoing(true);

	sector = getFlashDumpSector();
	page = getFlashDumpPage();

	local_flash_command = getFlashCommand();

	if(local_flash_command < HX_FLASH_MAX_VAL) {
		himax_HW_reset(HX_LOADCONFIG_DISABLE,HX_INT_DISABLE);
	}
	if ( i2c_himax_master_write(x81_command, 1, sizeof(x81_command), DEFAULT_RETRY_CNT) < 0 )//sleep out
	{
		TS_LOG_ERR("%s i2c write x81_command fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(HX_SLEEP_120MS);

	if ( i2c_himax_master_write(x82_command, 1, sizeof(x82_command), DEFAULT_RETRY_CNT) < 0 )
	{
		TS_LOG_ERR("%s i2c write x81_command fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(HX_SLEEP_100MS);

	TS_LOG_INFO("%s: local_flash_command = %d enter.\n", __func__,local_flash_command);

	if ((local_flash_command == FW_SHOW|| local_flash_command == FW_DUMP)|| (local_flash_command==0x0F))
	{
		/*set flash dump enable:D0 bit*/
		x43_command[1] = FW_SHOW;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0)
		{
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		for( i=0 ; i<FW_ROW ;i++)
		{
			for(j=0 ; j < FW_COL ; j++)
			{
				memset(page_tmp,0x00,sizeof(page_tmp));
				himax_ts_flash_work_func_case1(i,j,page_tmp);

				TS_LOG_INFO("%s: Run new method\n",__func__);
				memcpy(&flash_buffer[buffer_ptr],page_tmp,HX_RECEIVE_BUF_MAX_SIZE);
				buffer_ptr += HX_RECEIVE_BUF_MAX_SIZE;
				setFlashDumpProgress(i*FW_COL + j);
			}
		}
	}
	else if (local_flash_command == FL_SHOW)
	{
		/*set flash dump enable*/
		x43_command[1] = FW_SHOW;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		memset(page_tmp,0x00,sizeof(page_tmp));

		himax_ts_flash_work_func_case2(page,sector,page_tmp);

		TS_LOG_INFO("%s: Run new method\n",__func__);
		memcpy(&flash_buffer[buffer_ptr],page_tmp,HX_RECEIVE_BUF_MAX_SIZE);
		buffer_ptr += HX_RECEIVE_BUF_MAX_SIZE;
	}
	else if (local_flash_command == FL_DUMP)
	{
		himax_lock_flash(0);

		msleep(HX_SLEEP_50MS);

		/*page erase*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set flash manual_0*/
		x43_command[2] = (uint8_t)((X43_COMMAND_PAGE_ERASE >> SHIFT_ONE_BYTE) & HX_MASK_VALUE);
		/*set flash manual_1*/
		x43_command[3] = (uint8_t)((X43_COMMAND_PAGE_ERASE >> SHIFT_TWO_BYTE) & HX_MASK_VALUE);;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 3, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash addr*/
		x44_command[1] = HX_DATA_INIT_VAL;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		if ( i2c_himax_write_command(x4D_command[0], DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		/*enter manual mode*/
		x35_command[1] = HX_FLASH_MANUAL_MODE_EN;
		if( i2c_himax_write(x35_command[0],&x35_command[1], 1, sizeof(x35_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_100MS);

		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set flash manual*/
		x43_command[2] = (uint8_t)((X43_COMMAND_PAGE_ERASE >> SHIFT_ONE_BYTE) & HX_MASK_VALUE);
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		/*set flash address*/
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(x44_command[0],&x44_command[1], sizeof(x44_command), 3, DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		/*manual mode command : 47 to latch the flash address when page address change*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set ctrl flash register*/
		x43_command[2] = HX_FLASH_CTRL_CMD_9;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set ctrl flash register*/
		x43_command[2] = HX_FLASH_CTRL_CMD_D;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set ctrl flash register*/
		x43_command[2] = HX_FLASH_CTRL_CMD_9;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(HX_SLEEP_10MS);

		for(i = 0; i < FW_COL; i++)
		{
			TS_LOG_INFO("himax :i=%d \n",i);
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;
			if ( i2c_himax_write( x44_command[0],&x44_command[1], 3, sizeof(x44_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);
			m = i*4;
			x45_command[1] = flash_buffer[m];
			m+=1;
			x45_command[2] = flash_buffer[m];
			m+=1;
			x45_command[3] = flash_buffer[m];
			m+=1;
			x45_command[4] = flash_buffer[m];
			if ( i2c_himax_write( x45_command[0],&x45_command[1], 4, sizeof(x45_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 45 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);

			// manual mode command : 48 ,data will be written into flash buffer
			x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
			x43_command[2] = HX_FLASH_CTRL_CMD_D;
			if ( i2c_himax_write( x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(HX_SLEEP_10MS);
			/*set flash enable*/
			x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
			x43_command[2] = HX_FLASH_CTRL_CMD_D;
			if ( i2c_himax_write(x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
			{
				TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			msleep(HX_SLEEP_10MS);
		}

		// manual mode command : 49 ,program data from flash buffer to this page
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		x43_command[2] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		x43_command[2] = HX_FLASH_CTRL_CMD_5;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		/*set flash manual*/
		x43_command[2] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);
		/*set flash enable*/
		x43_command[1] = (uint8_t)(X43_COMMAND_PAGE_ERASE & HX_MASK_VALUE);
		x43_command[2] = HX_DATA_INIT_VAL;
		if ( i2c_himax_write(x43_command[0],&x43_command[1], 2, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(HX_SLEEP_10MS);

		// flash disable
		x43_command[1] = HX_DATA_INIT_VAL;
		if ( i2c_himax_write( x43_command[0],&x43_command[1], 1, sizeof(x43_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(HX_SLEEP_10MS);

		// leave manual mode
		x35_command[1] = HX_FLASH_MANUAL_MODE_DIS;
		if( i2c_himax_write( x35_command[0],&x35_command[1], 1, sizeof(x35_command), DEFAULT_RETRY_CNT) < 0 )
		{
			TS_LOG_ERR("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(HX_SLEEP_10MS);

		// lock flash
		himax_lock_flash(1);
		msleep(HX_SLEEP_50MS);

		buffer_ptr = HX_RECEIVE_BUF_MAX_SIZE;
		TS_LOG_INFO("Himax: Flash page write Complete\n");
	}

	TS_LOG_INFO("Complete\n");
	if(local_flash_command==FW_SHOW)
		{
		himax_ts_flash_work_func_print_buffer(buffer_ptr);

		}

	retval = i2c_himax_master_write( x43_command, 1, sizeof(x43_command), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("%s: i2c write fail\n", __func__);
	}
	msleep(HX_SLEEP_50MS);

	if (local_flash_command == FW_DUMP)
	{
		struct file *fn;

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,buffer_ptr*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}

	if(local_flash_command < HX_FLASH_MAX_VAL)
		himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	TS_LOG_INFO("%s: End\n",__func__);
	return;

Flash_Dump_i2c_transfer_error:

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
}

static ssize_t himax_flash_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int retval = 0;
	int loop_i = 0;
	uint8_t local_flash_fail = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_read_step = 0;
	int m = 0;
	local_flash_complete = getFlashDumpComplete();
	local_flash_progress = getFlashDumpProgress();
	local_flash_command = getFlashCommand();
	local_flash_fail = getFlashDumpFail();
	TS_LOG_INFO("flash_progress = %d \n",local_flash_progress);
	if (local_flash_fail)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart:Fail \n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		return retval;
	}

	if (!local_flash_complete)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart:Ongoing:0x%2.2x \n",flash_progress);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		return retval;
	}

	if (local_flash_command == FW_SHOW&& local_flash_complete)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart:Complete \n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		return retval;
	}

	if (local_flash_command == FL_SHOW&& local_flash_complete)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart: \n");
		for(loop_i = 0; loop_i < HX_RECEIVE_BUF_MAX_SIZE; loop_i++)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "x%2.2x", flash_buffer[loop_i]);
			if ((loop_i % 16) == 15)
			{
				retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
			}
		}
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		return retval;
	}

	//flash command == 0 , report the data
	local_flash_read_step = getFlashReadStep();

	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart:%2.2x \n",local_flash_read_step);

	for (loop_i = 0; loop_i < 1024; loop_i++)
	{
		m = local_flash_read_step*1024 + loop_i;
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "x%2.2X", flash_buffer[m]);

		if ((loop_i % 16) == 15)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		}
	}

	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	return retval;
}

static ssize_t himax_flash_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	int base = 0;
	uint8_t loop_i = 0;
	char buf_tmp[6] = {0};
	unsigned long result = 0;
	if (count >= 80)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	TS_LOG_INFO("%s: buf[0] = %s\n", __func__, buf);
	if (getSysOperation() == 1)
	{
		TS_LOG_ERR("%s: SYS is busy , return!\n", __func__);
		return count;
	}

	if (buf[0] == '0')
	{
		setFlashCommand(0);
		if (buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			TS_LOG_INFO("%s: read_Step = %s\n", __func__, buf_tmp);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				TS_LOG_INFO("%s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	}
	else if (buf[0] == '1')
	{
		setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		queue_work(g_himax_ts_data->flash_wq, &g_himax_ts_data->flash_work);
	}
	else if (buf[0] == '2')
	{
		setSysOperation(1);//step 1
		setFlashCommand(2);//step 2
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		queue_work(g_himax_ts_data->flash_wq, &g_himax_ts_data->flash_work);
	}
	else if (buf[0] == '3')
	{
		setSysOperation(1);
		setFlashCommand(3);//step 3
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!kstrtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!kstrtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}

		queue_work(g_himax_ts_data->flash_wq, &g_himax_ts_data->flash_work);
	}
	else if (buf[0] == '4')
	{
		TS_LOG_INFO("%s: command 4 enter.\n", __func__);
		setSysOperation(1);
		setFlashCommand(4);//step 4
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}
		else
		{
			TS_LOG_ERR("%s: command 4 , sector error.\n", __func__);
			return count;
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}
		else
		{
			TS_LOG_ERR("%s: command 4 , page error.\n", __func__);
			return count;
		}

		base = 11;

		TS_LOG_INFO("=========Himax flash page buffer start=========\n");
		for(loop_i=0;loop_i<HX_RECEIVE_BUF_MAX_SIZE;loop_i++)
		{
			memcpy(buf_tmp, buf + base, 2);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				flash_buffer[loop_i] = result;
				TS_LOG_INFO("%d ",flash_buffer[loop_i]);
				if (loop_i % 16 == 15)
				{
					TS_LOG_INFO("\n");
				}
			}
			base += 3;
		}
		TS_LOG_INFO("=========Himax flash page buffer end=========\n");

		queue_work(g_himax_ts_data->flash_wq, &g_himax_ts_data->flash_work);
	}
	return count;
}
static DEVICE_ATTR(flash_dump, (S_IWUSR|S_IRUGO),himax_flash_show, himax_flash_store); //Debug_flash_dump_done
#endif
#ifdef HX_TP_SYS_SELF_TEST
int himax_chip_self_test(void)
{
	int i = 0;
	int pf_value = 0x03;
	uint8_t cmdbuf[11] = {0};
	uint8_t valuebuf[16] = {0};
	int retval = 0;
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	memset(valuebuf, 0x00, sizeof(valuebuf));

	retval = i2c_himax_write( HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_50MS);
	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_50MS);
	cmdbuf[0] = 0x00;
	retval = i2c_himax_write( HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval <0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0x96;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);

	valuebuf[0] = 0x00;
	valuebuf[1] = 0x00;
	valuebuf[2] = 0xfb;//bank mul
	valuebuf[3] = 0x01;
	valuebuf[4] = 0xfe;//bank average
	valuebuf[5] = 0x01;
	valuebuf[6] = 0xfb;//bank self
	valuebuf[7] = 0x05;
	retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &valuebuf[0], 8, sizeof(valuebuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}

	cmdbuf[0] = 0x06;
	retval = i2c_himax_write( HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_50MS);

	retval = i2c_himax_write( HX_CMD_TSSON,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_30MS);

	retval = i2c_himax_write( HX_CMD_TSSLPOUT,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_5S);

	retval = i2c_himax_write(HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_50MS);

	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_30MS);

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	/*set read addr*/
	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0x96;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE, valuebuf, 8, sizeof(valuebuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_30MS);

	if (valuebuf[0]==0xAA) {
		TS_LOG_INFO("[Himax]: self-test pass\n");
		pf_value = 0x0;
	} else if((valuebuf[0] == 0xf1) || (valuebuf[0] == 0xf2) || (valuebuf[0] == 0xf3)){
		TS_LOG_ERR("[Himax]: self-test fail\n");
		pf_value = 0x1;
	}else{
		TS_LOG_ERR("[Himax]: self-test not completed\n");
	}

	for(i=0;i<8;i++) {
		TS_LOG_INFO("[Himax]: After self test buff_back FE[9%x]  = 0x%x\n",i+6,valuebuf[i]);
	}

	cmdbuf[0] = 0x02;
	retval = i2c_himax_write(HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = 0x00;
	retval = i2c_himax_write(HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return HX_ERROR;
	}
	msleep(HX_SLEEP_50MS);
	return pf_value;
}

static ssize_t himax_self_test_read(struct device *dev,struct device_attribute *attr, char *buf)
{
	int retval = 0;
	int val=0x00;

#ifdef HX_CHIP_STATUS_MONITOR
	int j=0;
#endif
       himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
#ifdef HX_CHIP_STATUS_MONITOR
		HX_CHIP_POLLING_COUNT=0;
		if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
		{
			for(j = 0; j < HX_HAND_SHAKING_MAX_TIME; j++)
				{
					if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
						{
							TS_LOG_INFO("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,j);
							break;
						}
					else
						msleep(HX_SLEEP_1MS);
				}

		}
		cancel_delayed_work_sync(&g_himax_ts_data->himax_chip_monitor);
#endif
	self_test_inter_flag= 1;
	msleep(HX_SLEEP_10MS);
	val = himax_chip_self_test();
	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif

#ifdef HIMAX_BANK_SELF_TEST
		ret += count_selftest;
		count_selftest = 0;
#endif
	if (val == 0x00) {
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Self_Test Pass\n");
	} else if (val == 0x01){
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Self_Test Fail\n");
	} else {
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Self_Test not complete\n");
	}
	self_test_inter_flag= 0;

	return retval;
}
static DEVICE_ATTR(tp_self_test, (S_IWUSR|S_IRUGO), himax_self_test_read, NULL); //Debug_selftest_done
#endif

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
static struct kobject *android_touch_kobj = NULL;// Sys kobject variable
#endif
static ssize_t himax_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	struct himax_ts_data *ts_data;
	ts_data = g_himax_ts_data;
	retval += sprintf(buf, "%d\n", ts_data->debug_log_level);
	return retval;
}

static ssize_t himax_debug_level_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int i = 0;
	struct himax_ts_data *ts = NULL;
	int m = (int)count;
	ts = g_himax_ts_data;
	ts->debug_log_level = 0;
	for(i=0; i<m-1; i++)
	{
		if( buf[i]>='0' && buf[i]<='9' )
			ts->debug_log_level |= (buf[i]-'0');
		else if( buf[i]>='A' && buf[i]<='F' )
			ts->debug_log_level |= (buf[i]-'A'+10);
		else if( buf[i]>='a' && buf[i]<='f' )
			ts->debug_log_level |= (buf[i]-'a'+10);

		if(i!=m-2)
			ts->debug_log_level <<= 4;
	}

	if (ts->debug_log_level & BIT(3)) {
		if (ts->pdata->screenWidth > 0 && ts->pdata->screenHeight > 0 &&
		 (ts->pdata->abs_x_max - ts->pdata->abs_x_min) > 0 &&
		 (ts->pdata->abs_y_max - ts->pdata->abs_y_min) > 0) {
		 if( (ts->pdata->abs_x_max - ts->pdata->abs_x_min) == 0|| (ts->pdata->abs_y_max - ts->pdata->abs_y_min) == 0 )
			{
				TS_LOG_ERR("%s: could not calculate!\n", __func__);
				return -EFAULT;
		 	}
			ts->widthFactor = (ts->pdata->screenWidth << SHIFTBITS)/(ts->pdata->abs_x_max - ts->pdata->abs_x_min);
			ts->heightFactor = (ts->pdata->screenHeight << SHIFTBITS)/(ts->pdata->abs_y_max - ts->pdata->abs_y_min);
			if (ts->widthFactor > 0 && ts->heightFactor > 0)
				ts->useScreenRes = 1;
			else {
				ts->heightFactor = 0;
				ts->widthFactor = 0;
				ts->useScreenRes = 0;
			}
		} else
			TS_LOG_INFO("Enable finger debug with raw position mode!\n");
	} else {
		ts->useScreenRes = 0;
		ts->widthFactor = 0;
		ts->heightFactor = 0;
	}
	return count;
}

static DEVICE_ATTR(debug_level, (S_IWUSR|S_IRUGO),	//Debug_level_done
	himax_debug_level_show, himax_debug_level_dump);


static ssize_t touch_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	struct himax_ts_data *ts_data;
	ts_data = g_himax_ts_data;
	retval += sprintf(buf, "%s_FW:%#x,%x_CFG:%#x_SensorId:%#x\n", HIMAX_CORE_NAME,
		ts_data->vendor_fw_ver_H, ts_data->vendor_fw_ver_L, ts_data->vendor_config_ver, ts_data->vendor_sensor_id);
	return retval;
}

static DEVICE_ATTR(vendor, (S_IRUGO), touch_vendor_show, NULL);	//Debug_vendor_done

static ssize_t touch_attn_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	struct himax_ts_data *ts_data;
	ts_data = g_himax_ts_data;
	retval += sprintf(buf, "%s_FW:%#x,%x_CFG:%#x_SensorId:%#x\n", HIMAX_CORE_NAME,
		ts_data->vendor_fw_ver_H, ts_data->vendor_fw_ver_L, ts_data->vendor_config_ver, ts_data->vendor_sensor_id);
	return retval;
}

static DEVICE_ATTR(attn, (S_IRUGO), touch_attn_show, NULL);	//Debug_attn_done
static ssize_t himax_int_status_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t retval = 0;
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", irq_enable_count);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	return retval;
}

static ssize_t himax_int_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;
	struct himax_ts_data *ts = g_himax_ts_data;

	if (count >= 12)
	{
		TS_LOG_INFO("%s: no command exceeds 12 chars.\n", __func__);
		return -EFAULT;
	}
	if (buf[0] == '0')
		value = false;
	else if (buf[0] == '1')
		value = true;
	else
		return -EINVAL;
	if (value) {
		himax_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,1);
		ts->irq_enabled = 1;
		irq_enable_count = 1;
	} else {
		himax_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,0);
		ts->irq_enabled = 0;
		irq_enable_count = 0;
	}
	return count;
}

static DEVICE_ATTR(int_en, (S_IWUSR|S_IRUGO),	//Debug_int_en_done
	himax_int_status_show, himax_int_status_store);
static ssize_t himax_layout_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct himax_ts_data *ts = g_himax_ts_data;
	size_t retval = 0;
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", ts->pdata->abs_x_min);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", ts->pdata->abs_x_max);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", ts->pdata->abs_y_min);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", ts->pdata->abs_y_max);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	return retval;
}

static ssize_t himax_layout_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	int i = 0, j = 0, k = 0;
	int layout[4] = {0};
	char buf_tmp[5] = {0};
	unsigned long value = 0;
	struct himax_ts_data *ts = g_himax_ts_data;

	for (i = 0; i < 20; i++) {
		if (buf[i] == ',' || buf[i] == '\n') {
			memset(buf_tmp, 0x0, sizeof(buf_tmp));
			if (i - j <= 5)
				memcpy(buf_tmp, buf + j, i - j);
			else {
				TS_LOG_INFO("buffer size is over 5 char\n");
				return count;
			}
			j = i + 1;
			if (k < 4) {
				retval = strict_strtol(buf_tmp, 10, &value);
				layout[k++] = value;
			}
		}
	}
	if (k == 4) {
		ts->pdata->abs_x_min=layout[0];
		ts->pdata->abs_x_max=layout[1];
		ts->pdata->abs_y_min=layout[2];
		ts->pdata->abs_y_max=layout[3];
		TS_LOG_INFO("%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
		input_unregister_device(ts->input_dev);
		retval=himax_input_config(ts->input_dev);
		if(retval)
		{
			TS_LOG_ERR("himax_input_config error\n");
		}
		retval=input_register_device(ts->input_dev);
		if(retval)
		{
			TS_LOG_ERR("input_register_device error\n");
		}
	} else
		TS_LOG_INFO("ERR@%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
	return count;
}

static DEVICE_ATTR(layout, (S_IWUSR|S_IRUGO), //Debug_layout_done
	himax_layout_show, himax_layout_store);

int himax_touch_sysfs_init(void)
{
	int ret;
	android_touch_kobj = kobject_create_and_add("himax_debug", NULL);
	if (android_touch_kobj == NULL) {
		TS_LOG_ERR("%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug_level.attr);
	if (ret) {
		TS_LOG_ERR("%s: create_file dev_attr_debug_level failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret) {
		TS_LOG_ERR("%s: sysfs_create_file dev_attr_vendor failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_reset.attr);
	if (ret) {
		TS_LOG_ERR("%s: sysfs_create_file dev_attr_reset failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_attn.attr);
	if (ret) {
		TS_LOG_ERR("%s: sysfs_create_file dev_attr_attn failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_int_en.attr);
	if (ret) {
		TS_LOG_ERR("%s: sysfs_create_file dev_attr_enabled failed\n", __func__);
		return ret;
	}

	ret = sysfs_create_file(android_touch_kobj, &dev_attr_layout.attr);
	if (ret) {
		TS_LOG_ERR("%s: sysfs_create_file dev_attr_layout failed\n", __func__);
		return ret;
	}
	#ifdef HX_TP_SYS_REGISTER
	register_command = 0;
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_register.attr);
	if (ret)
	{
		TS_LOG_ERR("create_file dev_attr_register failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_DIAG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr);
	if (ret)
	{
		TS_LOG_ERR("sysfs_create_file dev_attr_diag failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_DEBUG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug.attr);
	if (ret)
	{
		TS_LOG_ERR("create_file dev_attr_debug failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_FLASH_DUMP
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	if (ret)
	{
		TS_LOG_ERR("sysfs_create_file dev_attr_flash_dump failed\n");
		return ret;
	}
	#endif

	#ifdef HX_TP_SYS_SELF_TEST
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	if (ret)
	{
		TS_LOG_ERR("sysfs_create_file dev_attr_tp_self_test failed\n");
		return ret;
	}
	#endif

	return 0 ;
}



 void himax_touch_sysfs_deinit(void)
{
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug_level.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_reset.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_attn.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_int_en.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_layout.attr);

	#ifdef HX_TP_SYS_REGISTER
	sysfs_remove_file(android_touch_kobj, &dev_attr_register.attr);
	#endif

	#ifdef HX_TP_SYS_DIAG
	sysfs_remove_file(android_touch_kobj, &dev_attr_diag.attr);
	#endif

	#ifdef HX_TP_SYS_DEBUG
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug.attr);
	#endif

	#ifdef HX_TP_SYS_FLASH_DUMP
	sysfs_remove_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	#endif

	#ifdef HX_TP_SYS_SELF_TEST
	sysfs_remove_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	#endif

	kobject_del(android_touch_kobj);
}

int hx852xes_dbg_func_init(void)
{
	TS_LOG_INFO("%s: Entering!\n", __func__);
	himax_ts_flash_work_func = hx852xes_ts_flash_work_func;
	TS_LOG_INFO("%s: End!\n", __func__);
	return NO_ERR;
}
int hx852xf_dbg_func_init(void)
{
	TS_LOG_INFO("%s: Entering!\n", __func__);
	himax_ts_flash_work_func = hx852xf_ts_flash_work_func;
	TS_LOG_INFO("%s: End!\n", __func__);
	return NO_ERR;
}
