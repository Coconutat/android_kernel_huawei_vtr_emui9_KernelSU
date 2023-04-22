/**************************************************************************//**
 *
 *  @file		tuner_drv_hw.h
 *
 *  @brief		Common header file of the HardWare control layer.
 *
 *  @data		2014.07.10
 *
 *  @author	H.Kawano(KXDA3)
 *  @author	K.Okawa(KXDA3)
 *
 ***************************************************************************//*
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
#ifndef _TUNER_DRV_HW_H
#define _TUNER_DRV_HW_H

/******************************************************************************
 * include
 ******************************************************************************/
#include "tuner_drv.h"
#include <linux/gpio.h>
#include <linux/delay.h>

#define TUNER_CHECK_ERROR -1
#define TUNER_CHECK_OK 0

#define TUNER_DRV_OK 0 
#define TUNER_DRV_ERROR -1

#define TUNER_CHECK_I2C_NUM 1

#define TUNER_CHECK_I2C_READ_BANK 1
#define TUNER_CHECK_I2C_READ_ADDR 0xFF 
#define TUNER_CHECK_I2C_READ_VALUE 0x53 

#define TUNER_CHECK_I2C_WRITE_BANK 1 
#define TUNER_CHECK_I2C_WRITE_ADDR 0x00
#define TUNER_CHECK_I2C_WRITE_INIT 0x00
#define TUNER_CHECK_I2C_WRITE_VALUE 0x80

#define TUNER_CHECK_DELAY_TIME 1500

enum __tuner_gpio_type {
	TUNER_DRV_GPIO_POWER = 0,
	TUNER_DRV_GPIO_POWER_1V8 = 1,
	TUNER_DRV_GPIO_POWER_NRST = 2,
	TUNER_DRV_GPIO_POWER_MAX
};

enum __tuner_gpio_status {
	TUNER_DRV_POWER_GPIO_DISABLE = 0,
	TUNER_DRV_POWER_GPIO_ENABLE = 1
};

struct spi_drvdata {
        struct spi_device *spi;
        spinlock_t spi_lock;
        uint32_t gpio_cs;
};

/******************************************************************************
 * prototype
 ******************************************************************************/

/* following functions are described in "tuner_drv_hw.c" */
int tuner_drv_hw_reqirq(void);
void tuner_drv_hw_freeirq(void);

/*
 * following functions are described in "tuner_drv_<if>.c"
 */
int tuner_drv_hw_read_reg(
		enum _reg_bank bank,
		uint8_t adr,
		uint16_t len,
		uint8_t *rd
		);
int tuner_drv_hw_write_reg(
		enum _reg_bank bank,
		uint8_t adr,
		uint16_t len,
		uint8_t *wd
		);
int tuner_drv_hw_rmw_reg(
		enum _reg_bank bank,
		uint8_t adr,
		uint8_t mask,
		uint8_t wd
		);
int tuner_drv_hw_setev(tuner_event_t *ev);
int tuner_drv_hw_relev(tuner_event_t *ev);

//int tuner_drv_hw_startup_seq(void);
//void tuner_drv_hw_endup_seq(void);

int tuner_drv_hw_set_id(tuner_cpathid_t cpath_id);
int tuner_drv_check_i2c_connection(void);
int tuner_drv_check_spi_connection(void);

//int tuner_drv_gpio_power_process(int type, int status);
void tuner_drv_power_control_startup(void);
void tuner_drv_power_control_endup(void);
void tuner_drv_power_control_init(void);

extern tuner_cpathid_t cpath_id;

/*
 * following functions are described in "tuner_drv_hw_<if>.c".
 * <if> is NOT "i2c".
 */
int tuner_drv_hw_tsif_set_tpm(void);
int tuner_drv_hw_tsif_register(void);
void tuner_drv_hw_tsif_unregister(void);

int tuner_drv_hw_tsif_set_cntxt(struct _tsif_cntxt *tc);
int tuner_drv_hw_tsif_config(struct _tsif_cntxt *tc);
int tuner_drv_hw_tsif_get_pkts(struct _tsif_cntxt *tc);
int tuner_drv_hw_tsif_get_dready(void);
int tuner_drv_hw_tsif_sync_pkt(void);

/*
   Common Setting for slave IF
   These setting are only valid when DPATH is enabled.
*/

extern struct snglreg slvif_cfgregs[];
/* register offset is below */
#define SLVIF_CFG_SLVINTEN   (0)
#define SLVIF_CFG_ISEGSEL    (1)
#define SLVIF_CFG_DOSET4     (2)
#define SLVIF_CFG_WATERLINE  (3)
#define SLVIF_CFG_BYTEORDER  (4)
#define SLVIF_CFG_PKTSYNCC3  (5)
#define SLVIF_CFG_IFPWDSET1  (6)


#define DTV_GPIO_ISDB_OP_OK 0

#define DTV_SPI_BITS_PER_WORD_VALUE  8 

#endif /* _TUNER_DRV_HW_H */
/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
