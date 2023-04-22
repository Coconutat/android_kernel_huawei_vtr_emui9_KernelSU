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

#ifndef HIMAX_PLATFORM_H
#define HIMAX_PLATFORM_H

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include "linux/interrupt.h"
#include "../../huawei_ts_kit_algo.h"
#include "../../huawei_ts_kit.h"

#define CONFIG_HMX_DB
#if defined(CONFIG_HMX_DB)
#include <linux/regulator/consumer.h>
#endif

#define ONE_BYTE_CMD 1
#define TWO_BYTE_CMD 2
#define FOUR_BYTE_CMD 4
#define HX_I2C_MAX_SIZE 	256
#define CONFIG_TOUCHSCREEN_HIMAX_DEBUG

struct himax_i2c_platform_data {
	int abs_x_min;
	int abs_x_max;
	int abs_x_fuzz;
	int abs_y_min;
	int abs_y_max;
	int abs_y_fuzz;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_pressure_fuzz;
	int abs_width_min;
	int abs_width_max;
	int screenWidth;
	int screenHeight;
	uint8_t cable_config[2];
	int gpio_irq;
	int gpio_reset;
	int gpio_3v3_en;
	int gpio_1v8_en;

	int (*power)(int on);
	void (*reset)(void);

#if defined(CONFIG_HMX_DB)
	int irq_gpio;
	int reset_gpio;
#endif
};
void himax_nc_int_enable(int irqnum, int enable);
void himax_nc_rst_gpio_set(int pinnum, uint8_t value);
void himax_nc_register_read(uint8_t *read_addr, int read_length, uint8_t *read_data);
void himax_flash_write_burst(uint8_t * reg_byte, uint8_t * write_data);
void himax_burst_enable(uint8_t auto_add_4_byte);
int himax_nc_write_read_reg(uint8_t *tmp_addr,uint8_t *tmp_data,uint8_t hb,uint8_t lb);
int i2c_himax_nc_read(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
int i2c_himax_nc_write(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry);
#endif
