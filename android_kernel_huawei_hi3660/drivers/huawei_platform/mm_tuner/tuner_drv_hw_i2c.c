/**************************************************************************//**
 *
 *  @file		tuner_drv_hw_i2c.c
 *
 *  @brief		Implementation of the hardware control layer in I2C.
 *
 *  @data		2014.07.18
 *
 *  @author	H.Kawano (KXDA3)
 *  @author	K.Okawa (KXDA3)
 *
 ****************************************************************************//*
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/

/******************************************************************************
 * include
 ******************************************************************************/
#include "tuner_drv_hw.h"

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/of_device.h>
#include <linux/device.h>

/******************************************************************************
 * global
 ******************************************************************************/

/******************************************************************************
 * data
 ******************************************************************************/
static const uint8_t g_slvadr[6] = {
  /* SINGLE or DVR Master */
  0x60,	/* Sub */
  0x61,	/* Main1 */
  0x62,	/* Main2 */
  /* DVR Slave */
  0x70,	/* Sub */
  0x71,	/* Main1 */
  0x72,	/* Main2 */
};


extern struct _mmtuner_cntxt g_cnt;

static struct work_struct tuner_start_work;

static void tuner_start_work_handler(struct work_struct *work)
{
	int ret = 0;
	
	g_cnt.i2c_driver_flag = DRIVER_UNLOAD;
	
	ret = tuner_drv_start();
	if(ret != 0){
		pr_err("tuner_drv_start fail\n") ;
		return;
	}

	ret =gpio_request(g_cnt.gpio_nreset,"tuner_nreset");
	if(ret != 0){
		pr_err("gpio request tuner_nreset fail\n") ;
		goto start_err ;
	}

	ret = gpio_direction_output(g_cnt.gpio_nreset,TUNER_DRV_POWER_GPIO_DISABLE); 
	if(ret != 0){	
		pr_err("gpio  tuner_nreset direction out fail\n");
		goto start_err ;
	}

	ret =gpio_request(g_cnt.gpio_power_v18,"tuner_power_v18");
	if(ret != 0){
		pr_err("gpio request tuner_power_v18 fail\n") ;
		goto nrest_request_err ;
	}

	ret = gpio_direction_output(g_cnt.gpio_power_v18,TUNER_DRV_POWER_GPIO_DISABLE); 
	if(ret != 0){	
		pr_err("gpio  tuner_power_v18 direction out fail\n");
		goto nrest_request_err ;
	}

	ret =gpio_request(g_cnt.gpio_power,"tuner_power");
	if(ret != 0){
		pr_err("gpio request tuner_power fail\n") ;
		goto powerv18_request_err ;
	}

	ret = gpio_direction_output(g_cnt.gpio_power,TUNER_DRV_POWER_GPIO_DISABLE); 
	if(ret != 0){	
		pr_err("gpio  tuner_power direction out fail\n");
		goto power_request_err ;
	}

	tuner_drv_power_control_init() ;

	g_cnt.i2c_driver_flag = DRIVER_LOAD;

	return;

power_request_err:
	gpio_free(g_cnt.gpio_power);
powerv18_request_err:
	gpio_free(g_cnt.gpio_power_v18);
nrest_request_err:
	gpio_free(g_cnt.gpio_nreset);
start_err:
	tuner_drv_end();
	g_cnt.i2c_driver_flag = DRIVER_UNLOAD;
	pr_err("tuner_drv_hw_startup_seq failed.\n");
}

/**************************************************************************//**
 * initialization control of a driver
 *
 * @date		2011.08.02
 * @author		K.Kitamura(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 ******************************************************************************/
static int tuner_drv_hw_i2c_probe(struct i2c_client *client,
            const struct i2c_device_id *id)
{
	int retval = 0;
	struct device_node *node = client->dev.of_node;
	uint32_t value = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        	pr_err("mn88553_i2c_probe i2c is not supported\n");
       	return -EIO;
	}
	
	retval = of_property_read_u32(node, "gpio_power", &value);
	if(retval){
		pr_err("get gpio_power dts failed.\n");
		return retval;
	}
	g_cnt.gpio_power = value;
	
	retval = of_property_read_u32(node, "gpio_power_v18", &value);
	if(retval){
		pr_err("get gpio_power_v18 dts failed.\n");
		return retval;
	}
	g_cnt.gpio_power_v18 = value;
	
	retval = of_property_read_u32(node, "gpio_nreset", &value);
	if(retval){
		pr_err("get gpio_nreset dts failed.\n");
		return retval;
	}
	g_cnt.gpio_nreset = value;
	
	INIT_WORK(&tuner_start_work, tuner_start_work_handler);
	
	schedule_work(&tuner_start_work);

	return retval;
	
}

static int tuner_drv_hw_i2c_remove(struct i2c_client *client)
{
	if(DRIVER_LOAD == g_cnt.i2c_driver_flag){
		
		tuner_drv_end() ;

		__gpio_set_value(g_cnt.gpio_nreset, 0);

		mdelay(TUNER_DELAY);

		__gpio_set_value(g_cnt.gpio_power_v18, 0);

		mdelay(TUNER_DELAY);

		__gpio_set_value(g_cnt.gpio_power, 0);

		gpio_free(g_cnt.gpio_nreset);
		gpio_free(g_cnt.gpio_power_v18);
		gpio_free(g_cnt.gpio_power);
	}
	return 0;
}

/******************************************************************************
 * code area
 ******************************************************************************/
static const struct of_device_id switch_mn88553_ids[] = {
    { .compatible = "huawei,mn88553" },
    {},
};

MODULE_DEVICE_TABLE(of, switch_mn88553_ids);


static const struct i2c_device_id mn88553_i2c_id[] = {
    { "mn88553", 0 },
    { }
};

static struct i2c_driver mn88553_i2c_driver = {
    .driver = {
        .name = "mn88553",
        .owner = THIS_MODULE,
	 .of_match_table = switch_mn88553_ids,
    },
    .probe      = tuner_drv_hw_i2c_probe,
    .remove	= tuner_drv_hw_i2c_remove,
    .id_table = mn88553_i2c_id,
};

/**************************************************************************//**
 * Register the I2C driver
 *
 * @date	2013.12.10
 *
 * @author T.Abe (FSI)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0					Normal end
 * @retval <0					error
 ******************************************************************************/
int tuner_drv_hw_i2c_register(void)
{
	int retval = 0;
	
	retval = i2c_add_driver(&mn88553_i2c_driver);
    	if(retval) {
		pr_info("device_create() failed.\n");
		return -ENOEXEC;
	}
		
	return retval;
	
}

/**************************************************************************//**
 * Unregister the I2C driver
 *
 * @date	2014.08.21
 *
 * @author K.Okawa (KXDA3)
 *
 ******************************************************************************/
void tuner_drv_hw_i2c_unregister(void)
{
	i2c_del_driver(&mn88553_i2c_driver);
}

/**************************************************************************//**
 * read some of registers continuously
 *
 * @date	2014.07.18
 *
 * @author H.Kawano (KXDA3)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0					Normal end
 * @retval -EINVAL			error
 *
 * @param [in] bank	register bank enumerator
 * @param [in] adr	register offset address
 * @param [in] len	continuous read length
 * @param [out] rd	pointer to the buffer
 ******************************************************************************/
int tuner_drv_hw_read_reg(enum _reg_bank bank, uint8_t adr, uint16_t len, uint8_t *rd)
{
	int retval = 0;
	int rc = 0;
	struct i2c_adapter *adap = NULL;
	struct i2c_msg msgs[2] = {{0}};
	uint16_t rem = 0; 
//	const int rlen_max = TUNER_CONFIG_I2C_CONT_MAX;

	/* 0-clear */
	memset( msgs, 0x00, sizeof(struct i2c_msg) * 2);

	/* get i2c adapter */
	adap = i2c_get_adapter(TUNER_CONFIG_I2C_BUSNUM);
	if (!adap) {
		pr_err("I2C device not found.\n");
		return -ENODEV;
	}

	msgs[0].addr = g_slvadr[(int)bank];
	msgs[0].flags = 0;	/* Write flag. To write offset address. */
	msgs[0].len = 1;
	msgs[0].buf = &adr;	/* offset address. */
	msgs[1].addr = g_slvadr[(int)bank];
	msgs[1].flags = I2C_M_RD;	/* Read flag */
	msgs[1].buf = rd;				/* read buffer */
	rem = len;

	do {
//		msgs[1].len = (!rlen_max || rem < rlen_max) ? rem : rlen_max;
		msgs[1].len = rem ;

		rc = i2c_transfer(adap, msgs, 2);
		if (rc < 0) {
			pr_err("I2C(R) transfer fail.\n");
			retval = rc;
			goto out;
		}

		adr += ((bank == Main1 || adr != TUNER_NO_INCR_ADDR) ? msgs[1].len : 0);
		msgs[1].buf += msgs[1].len;
		*msgs[0].buf = adr;
		rem -= msgs[1].len;
	} while (rem);

out:

	i2c_put_adapter(adap);
	return retval;
}

/**************************************************************************//**
 * write some of registers continuously
 *
 * @date	2011.07.18
 *
 * @author H.Kawano (KXDA3)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0					Normal end
 * @retval <0					error (refer the errno)
 *
 * @param [in] bank	register bank enumerator
 * @param [in] adr	start address for continuous write
 * @param [in] len	continuous write length
 * @param [out] wd	pointer to the write data array
 ******************************************************************************/
int tuner_drv_hw_write_reg(enum _reg_bank bank, uint8_t adr, uint16_t len, uint8_t *wd)
{
	int retval = 0;
	int rc = 0;
	struct i2c_adapter *adap =NULL;
	struct i2c_msg msgs[1]= {{0}};
	uint8_t *wbuf = NULL;
	uint16_t rem = 0;
//	const int wlen_max = TUNER_CONFIG_I2C_CONT_MAX;

//	BUG_ON(wlen_max == 1);

	/* 0-clear */
	memset( msgs, 0x00, sizeof(struct i2c_msg));

	wbuf = (uint8_t *)kmalloc(len + 1, GFP_KERNEL);	/* Add 1 byte for the offset address */
	if (!wbuf) {
		pr_err("Memory Allocation error.\n");
		return -ENOMEM;
	}

	/* get i2c adapter */
	adap = i2c_get_adapter(TUNER_CONFIG_I2C_BUSNUM);
	if (!adap) {
		pr_err("i2c device not found.\n");
		retval = -ENODEV;
		goto out;
	}

	/* write data: offset, wd[0], wd[1], ..., wd[len-1] */
	*wbuf = adr;					/* offset address */
	memcpy(wbuf +1, wd, len);	/* write data */

	/* set parameters to msgs */
	msgs[0].addr = g_slvadr[(int)bank];
	msgs[0].flags = 0;			/* Write flag */
	msgs[0].buf = wbuf;			/* write data with offset address */
	rem = len;
	do {
		uint16_t wlen;

//		msgs[0].len = (!wlen_max || rem < wlen_max) ? (rem + 1) : wlen_max;
		msgs[0].len = (rem + 1);
		wlen = msgs[0].len -1;

		rc = i2c_transfer(adap, msgs, 1);
		if (rc < 0) {
			pr_err("I2C(W) transfer fail.\n");
			retval = rc;
			goto out;
		}
		
		adr += ((bank == Main1 || adr != TUNER_NO_INCR_ADDR) ? wlen : 0);
		msgs[0].buf += wlen;
		*msgs[0].buf = adr;
		rem -= wlen;
	} while (rem);

out:

	kfree(wbuf);
	i2c_put_adapter(adap);

	return retval;
}

/**************************************************************************//**
 * set cpath id
 *
 * @date	2015.12.14
 *
 * @author H.Niizuma (IoT1)
 *
 * @retval 0					Normal end
 *
 * @param [in] id	cpath id
 ******************************************************************************/
int tuner_drv_hw_set_id(tuner_cpathid_t id){
        cpath_id = id;
	return 0;
}

/**************************************************************************//**
 * @chip I2C connection Test
 *
 * @date		2015.10.05
 * @author		M.Sumida
 *
 * @retval		TUNER_CHECK_OK      normal exit
 * @retval		TUNER_CHECK_ERR_I2C	READ operation error
 ******************************************************************************/

int tuner_drv_check_i2c_read_part(void)
{
	int ret = 0;
	uint8_t value = 0 ;

	ret = tuner_drv_hw_read_reg(TUNER_CHECK_I2C_READ_BANK, \
		TUNER_CHECK_I2C_READ_ADDR, TUNER_CHECK_I2C_NUM, &value) ;
	if(ret != 0){
		pr_err("tunerx read regs fail.");
		return ret;
	}
	
	if(value != TUNER_CHECK_I2C_READ_VALUE){
		pr_err("tunerx check i2c read is not ok.");
		ret = TUNER_CHECK_ERROR ;
	}
	
	return ret ;
}

int tuner_drv_check_i2c_write_part(void)
{
	int ret = TUNER_CHECK_OK;
	uint8_t value = TUNER_CHECK_I2C_WRITE_INIT ;
	uint8_t test_value = TUNER_CHECK_I2C_WRITE_VALUE ;

	ret = tuner_drv_hw_read_reg(TUNER_CHECK_I2C_WRITE_BANK,  \
		TUNER_CHECK_I2C_WRITE_ADDR, TUNER_CHECK_I2C_NUM, &value) ;
	if(ret != TUNER_CHECK_OK){
		pr_err("tunerx check i2c write process read  is fail.");
		return  ret ;
	}

	if(value != TUNER_CHECK_I2C_WRITE_INIT){
		pr_err("tunerx check i2c write process read value is fail.");
		return  TUNER_CHECK_ERROR ;
	}
	
	ret = tuner_drv_hw_write_reg(TUNER_CHECK_I2C_WRITE_BANK, \
		TUNER_CHECK_I2C_WRITE_ADDR, TUNER_CHECK_I2C_NUM, &test_value) ;
	if(ret != TUNER_CHECK_OK){
		pr_err("tunerx check i2c write process write is fail.");
		return  ret ;
	}

	ret = tuner_drv_hw_read_reg(TUNER_CHECK_I2C_WRITE_BANK,  \
		TUNER_CHECK_I2C_WRITE_ADDR, TUNER_CHECK_I2C_NUM, &value) ;
	if(ret != TUNER_CHECK_OK){
		pr_err("tunerx check i2c write process read  is fail.");
		return  ret ;
	}

	if(value != TUNER_CHECK_I2C_WRITE_VALUE){
		pr_err("tunerx check i2c write process write value is fail.");
		return TUNER_CHECK_ERROR ;
	}

	value = TUNER_CHECK_I2C_WRITE_INIT ;

	ret = tuner_drv_hw_write_reg(TUNER_CHECK_I2C_WRITE_BANK, \
		TUNER_CHECK_I2C_WRITE_ADDR, TUNER_CHECK_I2C_NUM, &value) ;
	if(ret != TUNER_CHECK_OK){
		pr_err("tunerx check i2c write process write 2 is fail.");
		return  ret ;
	}

	return ret ;
	
}

 int  tuner_drv_check_i2c_connection(void) 
{
  	int ret=TUNER_CHECK_OK;

  	ret = tuner_drv_check_i2c_read_part() ;

	if(ret == TUNER_CHECK_OK){
		ret = tuner_drv_check_i2c_write_part() ;
	}
	
	return ret;
}

/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
