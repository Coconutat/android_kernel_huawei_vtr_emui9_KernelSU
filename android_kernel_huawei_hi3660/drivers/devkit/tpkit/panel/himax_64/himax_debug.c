/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2017 Himax Corporation.
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
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>

bool DSRAM_Flag = false;
int Flash_Size = FW_SIZE_64k;
int self_test_nc_flag = 0;

#define COLUMNS_LEN 16
#define REG_COMMON_LEN 128
#define DIAG_COMMAND_MAX_SIZE 80
#define DIAG_INT_COMMAND_MAX_SIZE 12

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#ifdef HX_TP_SYS_DIAG
	static uint8_t x_channel = 0;
	static uint8_t y_channel = 0;
	static uint8_t diag_max_cnt = 0;
	static int g_switch_mode = 0;
	static int16_t * diag_self = NULL;
	static int16_t *diag_mutual = NULL;
	static int16_t *diag_mutual_new = NULL;
	static int16_t *diag_mutual_old = NULL;

	int Selftest_flag = 0;
	int g_diag_command = 0;
	uint8_t write_counter = 0;
	uint8_t write_max_count = 30;
	struct file *diag_sram_fn;
	#define IIR_DUMP_FILE "/sdcard/HX_IIR_Dump.txt"
	#define DC_DUMP_FILE "/sdcard/HX_DC_Dump.txt"
	#define BANK_DUMP_FILE "/sdcard/HX_BANK_Dump.txt"
#endif

#ifdef HX_TP_SYS_REGISTER
	static uint8_t register_command[4] = {0};
#endif

#ifdef HX_TP_SYS_DEBUG
	uint8_t cmd_set[4] = {0};
	uint8_t mutual_set_flag = 0;
	static int handshaking_result = 0;
	static bool fw_update_complete = false;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[FW_SIZE_64k] = {0};
#endif

#ifdef HX_TP_SYS_FLASH_DUMP
	static uint8_t *flash_buffer = NULL;
	static uint8_t flash_command = 0;
	static uint8_t flash_read_step = 0;
	static uint8_t flash_progress = 0;
	static uint8_t flash_dump_complete = 0;
	static uint8_t flash_dump_fail = 0;
	static uint8_t sys_operation = 0;
	static bool    flash_dump_going = false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);

	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);
	static void setFlashDumpGoing(bool going);
#endif
#endif

extern char himax_nc_project_id[];
extern int hx_nc_irq_enable_count;
extern struct himax_ts_data *g_himax_nc_ts_data;

extern int himax_nc_input_config(struct input_dev * input_dev);
extern int i2c_himax_nc_read(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
extern int i2c_himax_nc_write(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
extern void himax_nc_int_enable(int irqnum, int enable);
extern void himax_nc_register_read(uint8_t *read_addr, int read_length, uint8_t *read_data);
extern void himax_burst_enable(uint8_t auto_add_4_byte);
extern void himax_register_write(uint8_t *write_addr, int write_length, uint8_t *write_data);
extern void himax_flash_write_burst(uint8_t * reg_byte, uint8_t * write_data);
extern void himax_nc_rst_gpio_set(int pinnum, uint8_t value);
extern void himax_nc_reload_disable(int on);

#ifdef HX_TP_SYS_RESET
static ssize_t himax_reset_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '1')
	{
		himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 0);
		msleep(RESET_LOW_TIME);
		himax_nc_rst_gpio_set(g_himax_nc_ts_data->rst_gpio, 1);
		msleep(RESET_HIGH_TIME);
	}
	return count;
}
static DEVICE_ATTR(reset, (S_IWUSR|S_IRUGO),NULL, himax_reset_set);
#endif

#ifdef HX_TP_SYS_DIAG
int16_t *hx_nc_getMutualBuffer(void)
{
	return diag_mutual;
}
int16_t *hx_nc_getMutualNewBuffer(void)
{
	return diag_mutual_new;
}
int16_t *hx_nc_getMutualOldBuffer(void)
{
	return diag_mutual_old;
}
int16_t *hx_nc_getSelfBuffer(void)
{
	return diag_self;
}
int16_t hx_nc_getXChannel(void)
{
	return x_channel;
}
int16_t hx_nc_getYChannel(void)
{
	return y_channel;
}
int16_t hx_nc_getDiagCommand(void)
{
	return g_diag_command;
}
void hx_nc_setXChannel(uint8_t x)
{
	x_channel = x;
}
void hx_nc_setYChannel(uint8_t y)
{
	y_channel = y;
}
void hx_nc_setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(int16_t), GFP_KERNEL);
	if (diag_mutual == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
	}
}
void hx_nc_setMutualNewBuffer(void)
{
	diag_mutual_new = kzalloc(x_channel * y_channel * sizeof(int16_t), GFP_KERNEL);
	if (diag_mutual_new == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
	}
}
void hx_nc_setMutualOldBuffer(void)
{
	diag_mutual_old = kzalloc(x_channel * y_channel * sizeof(int16_t), GFP_KERNEL);
	if (diag_mutual_old == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
	}
}

void hx_nc_setSelfBuffer(void)
{
	diag_self = kzalloc((x_channel + y_channel) * sizeof(int16_t), GFP_KERNEL);
	if (diag_self == NULL) {
		TS_LOG_ERR("%s: kzalloc error.\n", __func__);
	}
}
void hx_nc_freeMutualBuffer(void)
{
	if (diag_mutual)
		kfree(diag_mutual);
	diag_mutual = NULL;
}

void hx_nc_freeMutualNewBuffer(void)
{
    if (diag_mutual_new)
    {
        kfree(diag_mutual_new);
    }

    diag_mutual_new = NULL;
}

void hx_nc_freeMutualOldBuffer(void)
{
    if (diag_mutual_old)
    {
        kfree(diag_mutual_old);
    }

    diag_mutual_old = NULL;
}

void hx_nc_freeSelfBuffer(void)
{
    if (diag_self)
    {
        kfree(diag_self);
    }

    diag_self = NULL;
}
static int himax_determin_diag_rawdata(int diag_command)
{
	return diag_command%10;
}
static int himax_determin_diag_storage(int diag_command)
{
	return diag_command/10;
}
static ssize_t himax_diag_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	uint32_t loop_i = 0;
	uint16_t flg = 0;
	uint16_t width = 0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	int dsram_type = 0;

	int loop_j = 0;;
	unsigned int index = 0;
	uint8_t info_data_hx102b[MUTUL_NUM_HX83102* 2] = {0};
	uint8_t info_data_hx112a[MUTUL_NUM_HX83112* 2] = {0};
	int16_t new_data = 0;

	dsram_type = g_diag_command /10;

	//check if devided by zero
	if (x_channel == 0)
	{
		TS_LOG_ERR("%s devided by zero.");
		return count;
	}

	mutual_num	= x_channel * y_channel;
	self_num		= x_channel + y_channel; //don't add KEY_COUNT
	width		= x_channel;
	count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"ChannelStart: %4d, %4d\n\n", x_channel, y_channel);

	// start to show out the raw data in adb shell
	if ((g_diag_command >= 1 && g_diag_command <= 3) || (g_diag_command == 7))
	{
		for (loop_i = 0; loop_i < mutual_num; loop_i++)
		{
			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,"%6d", diag_mutual[loop_i]);
			if ((loop_i % width) == (width - 1))
			{
				flg=width + loop_i/width;
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count, " %6d\n", diag_self[flg]);
			}
		}

		count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,  "\n");
		for (loop_i = 0; loop_i < width; loop_i++)
		{
			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%6d", diag_self[loop_i]);
			if (((loop_i) % width) == (width - 1))
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
		}
#ifdef HX_EN_SEL_BUTTON
		count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"\n");
		for (loop_i = 0; loop_i < HX_NC_BT_NUM; loop_i++)
			count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"%6d", diag_self[HX_NC_RX_NUM + HX_NC_TX_NUM + loop_i]);
#endif
		count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"ChannelEnd");
		count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"\n");
	}
	else if (g_diag_command == 8)
	{
		for (loop_i = 0; loop_i < 128 ;loop_i++)
		{
			if ((loop_i % 16) == 0)
				count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"LineStart:");
			count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"%4x", hx_nc_diag_coor[loop_i]);
			if ((loop_i % 16) == 15)
				count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"\n");
		}
	}
	else if (dsram_type > 0 && dsram_type <= 8)
	{
		himax_burst_enable(1);
		if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
			himax_nc_get_DSRAM_data(info_data_hx102b);
		else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
			himax_nc_get_DSRAM_data(info_data_hx112a);
		for (loop_i = 0,index = 0; loop_i < HX_NC_TX_NUM; loop_i++)
		{
			for (loop_j = 0; loop_j < HX_NC_RX_NUM; loop_j++)
			{
				if(HX_83102B_SERIES_PWON ==IC_NC_TYPE)
					new_data = (short)(info_data_hx102b[index + 1] << 8 | info_data_hx102b[index]);
				else if(HX_83112A_SERIES_PWON ==IC_NC_TYPE)
					new_data = (short)(info_data_hx112a[index + 1] << 8 | info_data_hx112a[index]);
				if(dsram_type == 1)
				{
					diag_mutual[loop_i * HX_NC_RX_NUM+loop_j] = new_data;
				}
				index += 2;
			}
		}
		for (loop_i = 0; loop_i < mutual_num; loop_i++)
		{
			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,"%6d", diag_mutual[loop_i]);
			if ((loop_i % width) == (width - 1))
			{
				flg=width + loop_i/width;
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count, " %6d\n", diag_self[flg]);
			}
		}

		count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,  "\n");
		for (loop_i = 0; loop_i < width; loop_i++)
		{
			count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "%6d", diag_self[loop_i]);
			if (((loop_i) % width) == (width - 1))
				count+=snprintf(buf+count, HX_MAX_PRBUF_SIZE-count,   "\n");
		}
		count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"ChannelEnd");
		count+=snprintf(buf, HX_MAX_PRBUF_SIZE-count,"\n");
	}

	return count;
}

static ssize_t himax_diag_dump(struct device *dev, struct device_attribute *attr,  const char *buff,  size_t len)
{
	uint8_t command[2] = {0x00, 0x00};
	int storage_type = 0; // 0: common , other: dsram
	int rawdata_type = 0; // 1:IIR,2:DC
	struct himax_ts_data *ts;

	ts = g_himax_nc_ts_data;

	if (len >= DIAG_COMMAND_MAX_SIZE)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	TS_LOG_INFO("%s:g_switch_mode = %d\n",__func__,g_switch_mode);

	if (buff[1] == 0x0A){
		g_diag_command =buff[0] - '0';
	}else{
		g_diag_command =(buff[0] - '0')*10 + (buff[1] - '0');
	}

	storage_type = himax_determin_diag_storage(g_diag_command);
	rawdata_type = himax_determin_diag_rawdata(g_diag_command);

	if(g_diag_command > 0 && rawdata_type == 0)
	{
		TS_LOG_INFO("[Himax]g_diag_command=0x%x ,storage_type=%d, rawdata_type=%d! Maybe no support!\n"
		,g_diag_command,storage_type,rawdata_type);
		g_diag_command = 0x00;
	}
	else
		TS_LOG_INFO("[Himax]g_diag_command=0x%x ,storage_type=%d, rawdata_type=%d\n",g_diag_command,storage_type,rawdata_type);

	if (storage_type == 0 && rawdata_type > 0 && rawdata_type < 8)
	{
		TS_LOG_INFO("%s,common\n",__func__);
		if(DSRAM_Flag)
		{
			//1. Clear DSRAM flag
			DSRAM_Flag = false;

			//2. Enable ISR
			himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

			/*3 FW leave sram and return to event stack*/
			himax_nc_return_event_stack();
		}

		if(g_switch_mode == 2)
		{
			himax_nc_idle_mode(0);
			g_switch_mode = himax_nc_switch_mode(0);
		}

		if(g_diag_command == 0x04)
		{
			g_diag_command = 0x00;
			command[0] = 0x00;
		}
		else
			command[0] = g_diag_command;
		himax_nc_diag_register_set(command[0]);
	}
	else if (storage_type > 0 && storage_type < 8 && rawdata_type > 0 && rawdata_type < 8)
	{
		TS_LOG_INFO("%s,dsram\n",__func__);

		diag_max_cnt = 0;
		memset(diag_mutual, 0x00, x_channel * y_channel * sizeof(int16_t)); //Set data 0 everytime

		//0. set diag flag
		if(DSRAM_Flag)
		{
			//(1) Clear DSRAM flag
			DSRAM_Flag = false;

			//(2) Enable ISR
			himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

			/*(3) FW leave sram and return to event stack*/
			himax_nc_return_event_stack();
		}
		/* close sorting if turn on*/
		if(g_switch_mode == 2)
		{
			himax_nc_idle_mode(0);
			g_switch_mode = himax_nc_switch_mode(0);
		}

		switch(rawdata_type)
		{
			case 1:
				command[0] = 0x09;  //IIR
				break;
			case 2:
				command[0] = 0x0A;  //RAWDATA
				break;
			case 7:
				command[0] = 0x0B;   //DC
				break;
			default:
				command[0] = 0x00;
				TS_LOG_ERR("%s: Sram no support this type !\n",__func__);
				break;
		}
		himax_nc_diag_register_set(command[0]);

		//1. Disable ISR
		himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);

		//Open file for save raw data log
		#if 0
		if (storage_type == 4)
		{
			switch (rawdata_type)
			{
				case 1:
					diag_sram_fn = filp_open(IIR_DUMP_FILE,O_CREAT | O_WRONLY ,0);
					break;
				case 2:
					diag_sram_fn = filp_open(DC_DUMP_FILE,O_CREAT | O_WRONLY ,0);
					break;
				case 3:
					diag_sram_fn = filp_open(BANK_DUMP_FILE,O_CREAT | O_WRONLY ,0);
					break;
				default:
					TS_LOG_INFO("%s raw data type is not true. raw data type is %d \n",__func__, rawdata_type);
			}
		}

		TS_LOG_INFO("%s: Start get raw data in DSRAM\n", __func__);
		if (storage_type == 4)
			msleep(HX_SLEEP_6S);
		#endif
		//3. Set DSRAM flag
		DSRAM_Flag = true;


	}
	else if(storage_type == 8)
	{
		TS_LOG_INFO("Soritng mode!\n");

		if(DSRAM_Flag)
		{
			//1. Clear DSRAM flag
			DSRAM_Flag = false;
			//2. Enable ISR
			himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
			//3. FW leave sram and return to event stack
			himax_nc_return_event_stack();
		}

		himax_nc_idle_mode(1);
		g_switch_mode = himax_nc_switch_mode(1);
		if(g_switch_mode == 2)
		{
			if(rawdata_type == 1)
				command[0] = 0x09; //IIR
			else if(rawdata_type == 2)
				command[0] = 0x0A; //DC
			else if(rawdata_type == 7)
				command[0] = 0x08; //BASLINE
			else
			{
				command[0] = 0x00;
				TS_LOG_ERR("%s: Now Sorting Mode does not support this command=%d\n",__func__,g_diag_command);
			}
			himax_nc_diag_register_set(command[0]);
		}

		DSRAM_Flag = true;

	}
	else
	{
		//set diag flag
		if(DSRAM_Flag)
		{
			TS_LOG_INFO("return and cancel sram thread!\n");
			//1. Clear DSRAM flag
			DSRAM_Flag = false;

			//2. Enable ISR
			himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

			//3. FW leave sram and return to event stack
			himax_nc_return_event_stack();
		}

		if(g_switch_mode == 2)
		{
			himax_nc_idle_mode(0);
			g_switch_mode = himax_nc_switch_mode(0);
		}

		if(g_diag_command != 0x00)
		{

			TS_LOG_ERR("[Himax]g_diag_command error!diag_command=0x%x so reset\n",g_diag_command);
			command[0] = 0x00;
			if(g_diag_command != 0x08)
				g_diag_command = 0x00;
			himax_nc_diag_register_set(command[0]);
		}
		else
		{
			command[0] = 0x00;
			g_diag_command = 0x00;
			himax_nc_diag_register_set(command[0]);
			TS_LOG_INFO("return to normal g_diag_command=0x%x\n",g_diag_command);
		}
	}
	return len;
}

static DEVICE_ATTR(diag, (S_IWUSR|S_IRUGO),himax_diag_show, himax_diag_dump); //Debug_diag_done

#endif

#ifdef HX_TP_SYS_REGISTER
static ssize_t himax_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	uint16_t loop_i = 0;
	uint16_t row_width = 16;
	uint8_t data[REG_COMMON_LEN] = {0};

	TS_LOG_INFO("himax_register_show: %02X,%02X,%02X,%02X\n", register_command[3],register_command[2],register_command[1],register_command[0]);
	himax_nc_register_read(register_command, REG_COMMON_LEN, data);

	ret += snprintf(buf, HX_MAX_PRBUF_SIZE-ret, "command:  %02X,%02X,%02X,%02X\n", register_command[3],register_command[2],register_command[1],register_command[0]);

	for (loop_i = 0; loop_i < REG_COMMON_LEN; loop_i++) {
		ret += snprintf(buf + ret, HX_MAX_PRBUF_SIZE-ret, "0x%2.2X ", data[loop_i]);
		if ((loop_i % row_width) == (row_width -1))
			ret += snprintf(buf + ret, HX_MAX_PRBUF_SIZE-ret, "\n");
	}
	ret += snprintf(buf + ret, HX_MAX_PRBUF_SIZE-ret, "\n");

	return ret;
}

static ssize_t himax_register_store(struct device *dev,struct device_attribute *attr, const char *buff, size_t count)
{
	char buf_tmp[DIAG_COMMAND_MAX_SIZE]= {0};
	char *data_str = NULL;
	uint8_t length = 0;
	uint8_t loop_i = 0;
	uint8_t flg_cnt = 0;
 	uint8_t byte_length = 0;
	uint8_t w_data[DIAG_COMMAND_MAX_SIZE] = {0};
	uint8_t x_pos[DIAG_COMMAND_MAX_SIZE] = {0};
	unsigned long result   = 0;


	TS_LOG_INFO("%s:buff = %s, line = %d\n",__func__,buff,__LINE__);
	if (count >= DIAG_COMMAND_MAX_SIZE)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	TS_LOG_INFO("%s:buff = %s, line = %d, %p\n",__func__,buff,__LINE__,&buff[0]);

	if ((buff[0] == 'r' || buff[0] == 'w') && buff[1] == ':' && buff[2] == 'x') {

		length = strlen(buff);

		TS_LOG_INFO("%s: length = %d.\n", __func__,length);
		for (loop_i = 0;loop_i < length; loop_i++) //find postion of 'x'
		{
			if(buff[loop_i] == 'x')
			{
				x_pos[flg_cnt] = loop_i;
				flg_cnt++;
			}
		}
		TS_LOG_INFO("%s: flg_cnt = %d.\n", __func__,flg_cnt);

		data_str = strrchr(buff, 'x');
		TS_LOG_INFO("%s: %s.\n", __func__,data_str);
		length = strlen(data_str+1) - 1;

		if (buff[0] == 'r')
		{
			memcpy(buf_tmp, data_str + 1, length);
			byte_length = length/2;
			if (!kstrtoul(buf_tmp, 16, &result))
			{
				for (loop_i = 0 ; loop_i < byte_length ; loop_i++)
				{
					register_command[loop_i] = (uint8_t)(result >> loop_i*8);
				}
			}
			TS_LOG_INFO("%s: buff[0] == 'r'\n", __func__);
		}
		else if (buff[0] == 'w')
		{
			memcpy(buf_tmp, buff + 3, length);
			if(flg_cnt < 3)
			{
				byte_length = length/2;
				if (!kstrtoul(buf_tmp, 16, &result)) //command
				{
					for (loop_i = 0 ; loop_i < byte_length ; loop_i++)
					{
						register_command[loop_i] = (uint8_t)(result >> loop_i*8);
					}
				}
				if (!kstrtoul(data_str + 1, 16, &result)) //data
				{
					for (loop_i = 0 ; loop_i < byte_length ; loop_i++)
					{
						w_data[loop_i] = (uint8_t)(result >> loop_i*8);
					}
				}
				himax_register_write(register_command, byte_length, w_data);
			TS_LOG_INFO("%s: buff[0] == 'w' && flg_cnt < 3\n", __func__);
			}
			else
			{
				byte_length = x_pos[1] - x_pos[0] - 2;
				for (loop_i = 0;loop_i < flg_cnt; loop_i++) //parsing addr after 'x'
				{
					memcpy(buf_tmp, buff + x_pos[loop_i] + 1, byte_length);
					//TS_LOG_INFO("%s: buf_tmp = %s\n", __func__,buf_tmp);
					if (!kstrtoul(buf_tmp, 16, &result))
					{
						if(loop_i == 0)
						{
							register_command[loop_i] = (uint8_t)(result);
							//TS_LOG_INFO("%s: register_command = %X\n", __func__,register_command[0]);
						}
						else
						{
							w_data[loop_i - 1] = (uint8_t)(result);
							//TS_LOG_INFO("%s: w_data[%d] = %2X\n", __func__,loop_i - 1,w_data[loop_i - 1]);
						}
					}
				}

				byte_length = flg_cnt - 1;
				himax_register_write(register_command, byte_length, &w_data[0]);
			TS_LOG_INFO("%s: buff[0] == 'w' && flg_cnt >= 3\n", __func__);
			}
		}
		else
			return count;

	}

	return count;
}

static DEVICE_ATTR(register, (S_IWUSR|S_IRUGO),himax_register_show, himax_register_store); //Debug_register_done

#endif
#ifdef HX_TP_SYS_DEBUG

static ssize_t himax_debug_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t retval = 0;

	if (debug_level_cmd == 't')
	{
		if (fw_update_complete)
		{
			retval += snprintf(buf, HX_MAX_PRBUF_SIZE-retval, "FW Update Complete ");
		}
		else
		{
			retval += snprintf(buf, HX_MAX_PRBUF_SIZE-retval, "FW Update Fail ");
		}
	}
	else if (debug_level_cmd == 'v')
	{
		himax_nc_read_TP_info();
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FW_VER = ");
           	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X \n",g_himax_nc_ts_data->vendor_fw_ver);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "CONFIG_VER = ");
          	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "0x%2.2X \n",g_himax_nc_ts_data->vendor_config_ver);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	}
	else if (debug_level_cmd == 'd')
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Himax Touch IC Information :\n");
		if (IC_NC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : D\n");
		}
		else if (IC_NC_TYPE == HX_85XX_E_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : E\n");
		}
		else if (IC_NC_TYPE == HX_85XX_ES_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : ES\n");
		}
		else if (IC_NC_TYPE == HX_85XX_F_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : F\n");
		}
		else if (IC_NC_TYPE == HX_83102B_SERIES_PWON)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type : HX83102B\n");
		}
		else
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Type error.\n");
		}

		if (IC_NC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : SW\n");
		}
		else if (IC_NC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : HW\n");
		}
		else if (IC_NC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum : CRC\n");
		}
		else
		{
			retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "IC Checksum error.\n");
		}

		//retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Interrupt : LEVEL TRIGGER\n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "RX Num : %d\n",HX_NC_RX_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "TX Num : %d\n",HX_NC_TX_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "BT Num : %d\n",HX_NC_BT_NUM);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "X Resolution : %d\n",HX_NC_X_RES);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Y Resolution : %d\n",HX_NC_Y_RES);
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "Max Point : %d\n",HX_NC_MAX_PT);
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
	char fileName[REG_COMMON_LEN]= {0};
	mm_segment_t oldfs;
	struct file* hx_filp = NULL;
	TS_LOG_INFO("%s: enter\n", __func__);

	if (count >= DIAG_COMMAND_MAX_SIZE)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if ( buf[0] == 'v') //firmware version
	{
		debug_level_cmd = buf[0];
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
		himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
		debug_level_cmd 		= buf[0];
		fw_update_complete	= false;

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
			himax_nc_HW_reset(HX_LOADCONFIG_DISABLE,HX_INT_EN);;

			if (hx_nc_fts_ctpm_fw_upgrade_with_fs(upgrade_fw, result, true) == 0)
			{
				TS_LOG_ERR("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
				fw_update_complete = false;
			}
			else
			{
				TS_LOG_INFO("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
				himax_nc_reload_disable(0);
				fw_update_complete = true;

				g_himax_nc_ts_data->vendor_fw_ver = (upgrade_fw[NC_FW_VER_MAJ_FLASH_ADDR]<<8 | upgrade_fw[NC_FW_VER_MIN_FLASH_ADDR]);
				g_himax_nc_ts_data->vendor_config_ver = upgrade_fw[NC_CFG_VER_MAJ_FLASH_ADDR]<<8 | upgrade_fw[NC_CFG_VER_MIN_FLASH_ADDR];
			}
			himax_nc_HW_reset(HX_LOADCONFIG_EN,HX_INT_EN);
			goto firmware_upgrade_done;
		}
	}

	firmware_upgrade_done:

	himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);

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

bool hx_nc_getFlashDumpGoing(void)
{
	return flash_dump_going;
}

void hx_nc_setFlashBuffer(void)
{
	flash_buffer = kzalloc(FLASH_SIZE * sizeof(uint8_t), GFP_KERNEL);
	memset(flash_buffer,0x00,FLASH_SIZE);
}

void hx_nc_freeFlashBuffer(void)
{
	if (flash_buffer)
		kfree(flash_buffer);
	flash_buffer = NULL;
}

void hx_nc_setSysOperation(uint8_t operation)
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

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

static void himax_ts_flash_print_func(void)
{
	int i = 0;
      int cloum=COLUMNS_LEN;

	for(i = 0; i < Flash_Size; i++)
	{
		if (i % cloum == 0)
		{
			if(flash_buffer[i] < COLUMNS_LEN)
				printk("hx_flash_dump: 0x0%X,",flash_buffer[i]);
			else
				printk("hx_flash_dump: 0x%2X,",flash_buffer[i]);
		}
		else
		{
			if(flash_buffer[i] < COLUMNS_LEN)
				printk("0x0%X,",flash_buffer[i]);
			else
				printk("0x%2X,",flash_buffer[i]);
			if (i % cloum==(cloum-1))
			{
				printk("\n");
			}
		}
	}
}

static void himax_ts_flash_func(void)
{
	uint8_t local_flash_command = 0;
	mm_segment_t old_fs;

	himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,0);
	setFlashDumpGoing(true);
	local_flash_command = getFlashCommand();


	msleep(HX_SLEEP_100MS);

	TS_LOG_INFO("%s: local_flash_command = %d enter.\n", __func__, local_flash_command);

	if ((local_flash_command == 1 || local_flash_command == 2)|| (local_flash_command==0x0F))
	{
		himax_nc_flash_dump_func(local_flash_command, Flash_Size, flash_buffer);
	}

	if (local_flash_command == 1)
		himax_ts_flash_print_func();
	TS_LOG_INFO("Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
#if 0
	if (local_flash_command == 2)
	{
		struct file *fn;

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			TS_LOG_INFO("%s create file and ready to write\n",__func__);
			fn->f_op->write(fn, flash_buffer, Flash_Size*sizeof(uint8_t), &fn->f_pos);
			set_fs(old_fs);
			filp_close(fn,NULL);
		}
		else{
			set_fs(old_fs);
		}
	}
#endif
	himax_nc_int_enable(g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->irq_id,1);
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	hx_nc_setSysOperation(0);
	return;

}

void himax_nc_ts_flash_work_func(struct work_struct *work)
{
	himax_ts_flash_func();
}

static ssize_t himax_flash_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int flg= 0;
	int retval = 0;
	int loop_i = 0;
	uint8_t local_flash_fail = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_read_step = 0;

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

	if (local_flash_command == 1 && local_flash_complete)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart:Complete \n");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashEnd");
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
		return retval;
	}

	if (local_flash_command == 3 && local_flash_complete)
	{
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "FlashStart: \n");
		for(loop_i = 0; loop_i < 128; loop_i++)
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
		flg = local_flash_read_step*1024 + loop_i;
		retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "x%2.2X", flash_buffer[flg]);

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
	char buf_tmp[6] = {0};
	unsigned long result = 0;

	if (count >= DIAG_COMMAND_MAX_SIZE)
	{
		TS_LOG_INFO("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	TS_LOG_INFO("%s: buf = %s\n", __func__, buf);

	if (getSysOperation() == 1)
	{
		TS_LOG_ERR("%s: PROC is busy , return!\n", __func__);
		return count;
	}

	if (buf[0] == '0')
	{
		setFlashCommand(0);
		if (buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			TS_LOG_INFO("%s: read_Step = %s\n", __func__, buf_tmp);
			if (!kstrtoul(buf_tmp, 16, &result))
			{
				TS_LOG_INFO("%s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	}
	else if (buf[0] == '1')// 1_32,1_60,1_64,1_24,1_28 for flash size 32k,60k,64k,124k,128k
	{
		hx_nc_setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		if ((buf[1] == '_' ) && (buf[2] == '3' ) && (buf[3] == '2' ))
		{
			Flash_Size = FW_SIZE_32k;
		}
		else if ((buf[1] == '_' ) && (buf[2] == '6' ))
		{
			if (buf[3] == '0')
			{
				Flash_Size = FW_SIZE_60k;
			}
			else if (buf[3] == '4')
			{
				Flash_Size = FW_SIZE_64k;
			}
		}
		else if ((buf[1] == '_' ) && (buf[2] == '2' ))
		{
			if (buf[3] == '4')
			{
				Flash_Size = FW_SIZE_124k;
			}
			else if (buf[3] == '8')
			{
				Flash_Size = FW_SIZE_128k;
			}
		}
		TS_LOG_INFO("%s: command = 1, Flash_Size = %d\n", __func__,Flash_Size);
		queue_work(g_himax_nc_ts_data->flash_wq, &g_himax_nc_ts_data->flash_work);
	}
	else if (buf[0] == '2') // 2_32,2_60,2_64,2_24,2_28 for flash size 32k,60k,64k,124k,128k
	{
		hx_nc_setSysOperation(1);
		setFlashCommand(2);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		if ((buf[1] == '_' ) && (buf[2] == '3' ) && (buf[3] == '2' ))
		{
			Flash_Size = FW_SIZE_32k;
		}
		else if ((buf[1] == '_' ) && (buf[2] == '6' ))
		{
			if (buf[3] == '0')
			{
				Flash_Size = FW_SIZE_60k;
			}
			else if (buf[3] == '4')
			{
				Flash_Size = FW_SIZE_64k;
			}
		}
		else if ((buf[1] == '_' ) && (buf[2] == '2' ))
		{
			if (buf[3] == '4')
			{
				Flash_Size = FW_SIZE_124k;
			}
			else if (buf[3] == '8')
			{
				Flash_Size = FW_SIZE_128k;
			}
		}
		TS_LOG_INFO("%s: command = 2, Flash_Size = %d\n", __func__,Flash_Size);
		queue_work(g_himax_nc_ts_data->flash_wq, &g_himax_nc_ts_data->flash_work);
	}
	return count;
}
static DEVICE_ATTR(flash_dump, (S_IWUSR|S_IRUGO),himax_flash_show, himax_flash_store); //Debug_flash_dump_done
#endif

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
static struct kobject *android_touch_kobj = NULL;// Sys kobject variable
#endif
static ssize_t himax_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	struct himax_ts_data *ts_data;
	ts_data = g_himax_nc_ts_data;
	retval += snprintf(buf, HX_MAX_PRBUF_SIZE-retval, "%d\n", ts_data->debug_log_level);
	return retval;
}

static ssize_t himax_debug_level_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int loop_i = 0;
	struct himax_ts_data *ts = NULL;
	int flg = (int)count;
	ts = g_himax_nc_ts_data;
	ts->debug_log_level = 0;
	for(loop_i=0; loop_i<flg-1; loop_i++)
	{
		if( buf[loop_i]>='0' && buf[loop_i]<='9' )
			ts->debug_log_level |= (buf[loop_i]-'0');
		else if( buf[loop_i]>='A' && buf[loop_i]<='F' )
			ts->debug_log_level |= (buf[loop_i]-'A'+10);
		else if( buf[loop_i]>='a' && buf[loop_i]<='f' )
			ts->debug_log_level |= (buf[loop_i]-'a'+10);

		if(loop_i!=flg-2)
			ts->debug_log_level <<= 4;
	}

	if (ts->debug_log_level & BIT(3)) {
		if (ts->pdata->screenWidth > 0 && ts->pdata->screenHeight > 0 &&
		 (ts->pdata->abs_x_max - ts->pdata->abs_x_min) > 0 &&
		 (ts->pdata->abs_y_max - ts->pdata->abs_y_min) > 0) {
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

static DEVICE_ATTR(debug_level, (S_IWUSR|S_IRUGO), himax_debug_level_show, himax_debug_level_dump);


static ssize_t touch_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	struct himax_ts_data *ts_data;
	ts_data = g_himax_nc_ts_data;
	himax_nc_read_TP_info();
	retval += snprintf(buf, HX_MAX_PRBUF_SIZE-retval, "%s_FW:%#x_CFG:%#x_ProjectId:%s\n", ts_data->tskit_himax_data->ts_platform_data->chip_data->chip_name,
		ts_data->vendor_fw_ver, ts_data->vendor_config_ver, himax_nc_project_id);
	return retval;
}

static DEVICE_ATTR(vendor, (S_IRUGO), touch_vendor_show, NULL);//Debug_vendor_done

static ssize_t touch_attn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t retval = 0;
	return retval;
}

static DEVICE_ATTR(attn, (S_IRUGO), touch_attn_show, NULL);	//Debug_attn_done
static ssize_t himax_int_status_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t retval = 0;
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "%d ", hx_nc_irq_enable_count);
	retval+=snprintf(buf+retval, HX_MAX_PRBUF_SIZE-retval,  "\n");
	return retval;
}

static ssize_t himax_int_status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;
	struct himax_ts_data *ts = g_himax_nc_ts_data;

	if (count >= DIAG_INT_COMMAND_MAX_SIZE)
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
		himax_nc_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,1);
		ts->irq_enabled = 1;
		hx_nc_irq_enable_count = 1;
	} else {
		himax_nc_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,0);
		ts->irq_enabled = 0;
		hx_nc_irq_enable_count = 0;
	}
	return count;
}

static DEVICE_ATTR(int_en, (S_IWUSR|S_IRUGO),	//Debug_int_en_done
	himax_int_status_show, himax_int_status_store);

int himax_nc_touch_sysfs_init(void)
{
	int ret = 0;
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

	#ifdef HX_TP_SYS_REGISTER
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

	return NO_ERR;
}

 void himax_nc_touch_sysfs_deinit(void)
{
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug_level.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_reset.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_attn.attr);
	sysfs_remove_file(android_touch_kobj, &dev_attr_int_en.attr);

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

	kobject_del(android_touch_kobj);
}
