/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include "hw_flash_i2c.h"

/*set len factor as 2, for both saving reg addr and data.*/
#define BUF_LEN_FACTOR    (2)
#define I2C_MAX_BUF_LEN   (16)//I2C_MAX_BUF_LEN > I2C_ADDR_LEN
#define I2C_ADDR_LEN      (1)
#define I2C_DATA_LEN      ((I2C_MAX_BUF_LEN)-(I2C_ADDR_LEN))

extern int memset_s(void *dest, size_t destMax, int c, size_t count);
extern int memcpy_s(void *dest, size_t destMax, const void *src, size_t count);

int hw_flash_i2c_read(struct hw_flash_i2c_client *client,
	u8 reg, u8 *data)
{
	int rc = 0;
	struct i2c_msg msgs[2];

	cam_debug("%s enter.\n", __func__);

	msgs[0].addr = client->client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].addr = client->client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = data;

	rc = i2c_transfer(client->client->adapter, msgs, 2);
	if (rc < 0) {
		cam_err("%s transfer error, reg=0x%x, data=0x%x.",
			__func__, reg, *data);
	} else {
		cam_debug("%s reg=0x%x, data=0x%x.\n", __func__, reg, *data);
	}

	return rc;
}

int hw_flash_i2c_write(struct hw_flash_i2c_client *client,
	u8 reg, u8 data)
{
	int rc = 0;
	u8 buf[2] = {0};
	struct i2c_msg msg = {0};

	cam_debug("%s reg=0x%x, data=0x%x.\n", __func__, reg, data);

	buf[0] = reg;
	buf[1] = data;
	msg.addr = client->client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	rc = i2c_transfer(client->client->adapter, &msg, 1);
	if (rc < 0) {
		cam_err("%s transfer error, reg=0x%x, data=0x%x.",
			__func__, reg, data);
	}

	return rc;
}

int hw_flash_i2c_block_write(struct hw_flash_i2c_client *client,
	u8 reg, u8 *wr_buf, int len)
{
    int rc = 0;
    u8 buf[I2C_MAX_BUF_LEN] = {0};
    struct i2c_msg msg = {0};

    if ((NULL == client) || (NULL == wr_buf)) {
        cam_err("%s invalid params, client or wr_buf is null", __func__);
        return -EINVAL;
    }

    if (len > I2C_DATA_LEN) {
        cam_err("%s invalid params, len(%d) > I2C_DATA_LEN(%d)", __func__,
            len, I2C_DATA_LEN);
        return -EINVAL;
    }

    buf[0] = reg;
    memcpy_s(&buf[I2C_ADDR_LEN], I2C_DATA_LEN, wr_buf, len);//I2C_MAX_BUF_LEN > I2C_ADDR_LEN

    msg.addr = client->client->addr;
    msg.flags = 0;
    msg.len = len + I2C_ADDR_LEN;
    msg.buf = buf;

    rc = i2c_transfer(client->client->adapter, &msg, 1);
    if (rc < 0) {
        cam_err("%s transfer error, reg=0x%x.",
            __func__, reg);
    }

    return rc;
}
/*lint +e679 */
struct hw_flash_i2c_fn_t hw_flash_i2c_func_tbl = {
	.i2c_read = hw_flash_i2c_read,
	.i2c_write = hw_flash_i2c_write,
	.i2c_write_block = hw_flash_i2c_block_write,
};
