/**************************************************************************//**
 *
 *  @file		tuner_drv_config.h
 *
 *  @brief		configuration header of the mm_tuner55x driver
 *
 *  @data		2011.08.28
 *
 *  @author	K.Kitamura(*)
 *  @author	K.Okawa(KXDA3)
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
#ifndef _TUNER_DRV_CONFIG_H
#define _TUNER_DRV_CONFIG_H

/******************************************************************************
 * define
 ******************************************************************************/
#define TUNER_SET_ON                     1       /* setting ON */
#define TUNER_SET_OFF                    0       /* setting OFF */

/* device driver file name */
#define TUNER_CONFIG_DRIVER_NAME		"mmtuner"

/* device number */
#define TUNER_CONFIG_DRV_MAJOR         99       /* MAJOR No. */

#define TUNER_CONFIG_DRV_MINOR         200       /* MINOR No. */

/**
 * IRQ kernel thread priority (0-99) */
#define TUNER_CONFIG_KTH_PRI			95

/* TS I/F kernel thread priority (0-99)  */
#define TUNER_CONFIG_TSBTH_PRI			95

/* exclusive access control  */
/* #define TUNER_CONFIG_DRV_MULTI */

/**
 * I2C Bus Number */
#define TUNER_CONFIG_I2C_BUSNUM			0x01		/* I2C Bus number */

/**
 * @brief I2C continuously access limit.
 *
 *  "0" means NO limit.
 *  You MUST NOT set negative and "1". */
#define TUNER_CONFIG_I2C_CONT_MAX		(0)

/**
 * @brief Compile switch for the SPI EDGE mode.
 *
 * Use SPI edge mode when using SPI slave I/F.
 * Please define this macro TUNER_CONFIG_SPI_EDGE when you use 30MHz over SPI CLK. */
 #define TUNER_CONFIG_SPI_EDGE 

/**
 * @brief SPI calibration Timeout [ms]. */
#define TUNER_CONFIG_CALIBRATION_TIMEOUT		(1800000UL)

/**
 * @brief Compile switch for the SPI BREAK code at every SPI command.
 *
 * Send SPI BREAKCODE.
 * Insert SPI break pattern before every SPI command when defined. */
 #define TUNER_CONFIG_SPI_BREAKCODE 

/**
 * @brief TS-Read timeout limit [ms]. */
#define TUNER_CONFIG_TSREAD_TIMEOUT			(1200UL)

#endif/* _TUNER_DRV_CONFIG_H */
/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
