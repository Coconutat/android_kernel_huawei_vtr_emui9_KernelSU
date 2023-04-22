/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software 
** Foundation; version 2.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
** Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
** File:
**     tas2560-misc.h
**
** Description:
**     header file for tas2560-misc.c
**
** =============================================================================
*/

#ifndef _TAS2560_MISC_H
#define _TAS2560_MISC_H

#define	TIAUDIO_CMD_REG_WITE			1
#define	TIAUDIO_CMD_REG_READ			2
#define	TIAUDIO_CMD_DEBUG_ON			3
#define	TIAUDIO_CMD_CALIBRATION			7
#define	TIAUDIO_CMD_SAMPLERATE			8
#define	TIAUDIO_CMD_BITRATE				9
#define	TIAUDIO_CMD_DACVOLUME			10
#define	TIAUDIO_CMD_SPEAKER				11

#define	TAS2560_MAGIC_NUMBER   'M'	/* '2560' */

#define	SMARTPA_SPK_DAC_VOLUME                     	(_IOWR('M', 1, unsigned int))
#define	SMARTPA_SPK_POWER_ON                        	(_IOWR('M', 2, unsigned int))
#define	SMARTPA_SPK_POWER_OFF                       	(_IOWR('M', 3, unsigned int))
#define	SMARTPA_SPK_SET_SAMPLERATE              	(_IOWR('M', 7, unsigned int))
#define	SMARTPA_SPK_SET_BITRATE                    	(_IOWR('M', 8, unsigned int))
#define	SMARTPA_SPK_MUTE                               	(_IOWR('M', 9, unsigned int))
#define	SMARTPA_SPK_UNMUTE                           	(_IOWR('M',10, unsigned int))
#define	SMARTPA_SPK_FADEIN                           	(_IOWR('M',11, unsigned int))
#define	SMARTPA_SPK_FADEOUT                          	(_IOWR('M',12, unsigned int))
#define	SMARTPA_SPK_FADEIN_OPEN            		(_IOWR('M',13, unsigned int))
#define	SMARTPA_SPK_FADEOUT_CLOSE         		(_IOWR('M',14, unsigned int))
#define	SMARTPA_RESET                                  	(_IOWR('M',15, unsigned int))
#define	SMARTPA_DUMP_REGS                          	(_IOWR('M',16, unsigned int))
#define	SMARTPA_RESET_CHIP                             	(_IOWR('M',17, unsigned int))

int tas2560_register_misc(struct tas2560_priv *pTAS2560);
int tas2560_deregister_misc(struct tas2560_priv *pTAS2560);

#endif /* _TAS2560_MISC_H */
