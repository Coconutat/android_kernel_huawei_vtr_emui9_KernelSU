/* Himax Android Driver Sample Code Ver for Himax chipset
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

#include "himax_platform.h"
#include "himax_ic.h"

int hx_nc_irq_enable_count = 1;
extern struct himax_ts_data *g_himax_nc_ts_data;

int i2c_himax_nc_read(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len,uint8_t toRetry)
{
	int retval = HX_ERR;
	uint16_t addr_length = 0;
	uint8_t cmd_input[1] = {0};
	struct ts_bus_info *bops = NULL;

	if(data == NULL) {
		return retval;
	}
	if(length > limit_len){
		TS_LOG_ERR("%s: i2c_read_block size error %d, over array size%d\n",
			__func__, length, limit_len);
		return retval;
	}
	bops = g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->bops;
	addr_length = 1;
	cmd_input[0] = command;

	retval = bops->bus_read(cmd_input, addr_length, data, length);
	if (retval<0)
	{
		mdelay(HX_SLEEP_5MS);
		TS_LOG_ERR("%s: i2c_read_block error %d, retry over %d\n",__func__, retval, toRetry);
	}
	return retval;
}

int i2c_himax_nc_write(uint8_t command,uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry)
{
	int retval = HX_ERR;
	uint8_t buf[HX_I2C_MAX_SIZE] = {0};
	struct ts_bus_info *bops = NULL;

	if(data == NULL) {
		return retval;
	}
	if((length > limit_len)||(length >= HX_I2C_MAX_SIZE)){
		TS_LOG_ERR("%s: i2c_write_block size error %d, over array size%d\n",
			__func__, length, limit_len);
		return retval;
	}
	bops = g_himax_nc_ts_data->tskit_himax_data->ts_platform_data->bops;

	buf[0] = command;
	memcpy(buf+1, data, length);

	retval = bops->bus_write(buf,length+1);

	if (retval<0)
	{
		mdelay(HX_SLEEP_5MS);
		TS_LOG_ERR("%s: i2c_write_block error %d, retry over %d\n",__func__, retval, toRetry);
	}
	return retval;
}

void himax_burst_enable(uint8_t auto_add_4_byte)
{
	uint8_t tmp_data[4] = {0};

	tmp_data[0] = DATA_EN_BURST_MODE;
	if ( i2c_himax_nc_write(ADDR_EN_BURST_MODE ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}

	tmp_data[0] = (DATA_AHB | auto_add_4_byte);
	if ( i2c_himax_nc_write(ADDR_AHB ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
}

void himax_flash_write_burst(uint8_t * reg_byte, uint8_t * write_data)
{
	int i = 0;
	int j = 0;
	uint8_t data_byte[8] = {0};

	for (i = 0; i < FOUR_BYTE_CMD; i++)
	{
		data_byte[i] = reg_byte[i];
	}
	for (j = 4; j < 2 * FOUR_BYTE_CMD; j++)
	{
		data_byte[j] = write_data[j-4];
	}

	if ( i2c_himax_nc_write(ADDR_FLASH_BURNED ,data_byte, 2 * FOUR_BYTE_CMD, sizeof(data_byte), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}

}

void himax_nc_flash_write_burst_lenth(uint8_t *reg_byte, uint8_t *write_data, int length)
{
	int i = 0;
	int j = 0;
	uint8_t data_byte[256] = {0};

	for (i = 0; i < FOUR_BYTE_CMD; i++)
	{
		data_byte[i] = reg_byte[i];
	}
	for (j = 4; j < length + 4; j++)
	{
		data_byte[j] = write_data[j - 4];
	}

	if ( i2c_himax_nc_write(ADDR_FLASH_BURNED ,data_byte, length + 4, sizeof(data_byte), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
}

void himax_nc_register_read(uint8_t *read_addr, int read_length, uint8_t *read_data)
{
	int i = 0;
	int address = 0;
	uint8_t tmp_data[4] = {0};

	if(read_length > MAX_READ_LENTH)
	{
		TS_LOG_ERR("%s: read len over 256!\n", __func__);
		return;
	}
	if (read_length > FOUR_BYTE_CMD)
		himax_burst_enable(1);
	else
		himax_burst_enable(0);

	address = (read_addr[3] << 24) + (read_addr[2] << 16) + (read_addr[1] << 8) + read_addr[0];
	i = address;
	tmp_data[0] = (uint8_t)i;
	tmp_data[1] = (uint8_t)(i >> 8);
	tmp_data[2] = (uint8_t)(i >> 16);
	tmp_data[3] = (uint8_t)(i >> 24);
	if ( i2c_himax_nc_write(ADDR_FLASH_BURNED ,tmp_data, FOUR_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
	tmp_data[0] = DATA_READ_ACCESS;
	if ( i2c_himax_nc_write(ADDR_READ_ACCESS ,tmp_data, ONE_BYTE_CMD, sizeof(tmp_data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_nc_read(DUMMY_REGISTER,read_data, read_length, MAX_READ_LENTH, DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
	if (read_length > FOUR_BYTE_CMD)
		himax_burst_enable(0);
}

void himax_register_write(uint8_t *write_addr, int write_length, uint8_t *write_data)
{
	int i =0;
	int address = 0;

	address = (write_addr[3] << 24) + (write_addr[2] << 16) + (write_addr[1] << 8) + write_addr[0];

	for (i = address; i < address + write_length; i++)
	{
		if (write_length > FOUR_BYTE_CMD)
		{
			himax_burst_enable(1);
		}
		else
		{
			himax_burst_enable(0);
		}
		himax_nc_flash_write_burst_lenth(write_addr, write_data, write_length);
	}
}

int himax_nc_write_read_reg(uint8_t *tmp_addr,uint8_t *tmp_data,uint8_t hb,uint8_t lb)
{
	int cnt = 0;

	do{
		himax_flash_write_burst(tmp_addr, tmp_data);
		msleep(HX_SLEEP_10MS);
		himax_nc_register_read(tmp_addr, FOUR_BYTE_CMD, tmp_data);

	}while((tmp_data[1] != hb && tmp_data[0] != lb) && cnt++ < 100);

	if(cnt == 99)
		return -1;

	TS_LOG_DEBUG("Now register 0x%08X : high byte=0x%02X,low byte=0x%02X\n",tmp_addr[3],tmp_data[1],tmp_data[0]);
	return NO_ERR;
}

void himax_nc_int_enable(int irqnum, int enable)
{
	TS_LOG_INFO("S_irqnum=%d, irq_enable_count = %d, enable =%d\n",irqnum, hx_nc_irq_enable_count, enable);

	if (enable == 1 && hx_nc_irq_enable_count == 0) {
		enable_irq(irqnum);
		hx_nc_irq_enable_count=1;
	} else if (enable == 0 && hx_nc_irq_enable_count == 1) {
		disable_irq_nosync(irqnum);
		hx_nc_irq_enable_count=0;
	}
	TS_LOG_INFO("E_irqnum=%d, irq_enable_count = %d, enable =%d\n",irqnum, hx_nc_irq_enable_count, enable);
}
void himax_nc_rst_gpio_set(int pinnum, uint8_t value)
{
//	GTP_GPIO_OUTPUT(pinnum, value);
	gpio_direction_output(pinnum, value);
}
