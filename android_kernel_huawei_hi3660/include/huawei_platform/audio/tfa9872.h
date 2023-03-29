/*
 * max98925.h -- TFA9872 ALSA SoC Audio driver
 *
 * Copyright 2011-2012 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TFA9872_H
#define _TFA9872_H

//#include <linux/version.h>

/* One can override the Linux version here with an explicit version number */
/* #define TFA9872_LINUX_VERSION LINUX_VERSION_CODE */
/* #define TFA9872_LINUX_VERSION KERNEL_VERSION(3,4,0) */

/* Maximum number of TFA9872 devices in the system this driver can support */
#define MAX_NUM_TFA9872						2
#define TFA9872_MIN_VOLUME					(6)
#define TFA9872_MAX_VOLUME					(21)
#define TFA9872_HOLD_TIME					(1)
#define TFA9872_RESET_TIME					(1)

/*
 * version
 */
#define TFA9872_VERSION_REG					(0x03)
#define TFA9872_VERSION_N1A					(0x1A72)
#define TFA9872_VERSION_N1B					(0x3B72)

/*
 * TFA9872 Register Definitions
 */
#define TFA9872_SYS_CONTROL0				(0x00)
#define TFA9872_SYS_CTRL0_MASK_DCA			(1<<4)
#define TFA9872_SYS_CTRL0_MASK_AMPE			(1<<3)
#define TFA9872_SYS_CTRL0_MASK_PWDN			(1<<0)

#define TFA9872_SYS_CONTROL1				(0x01)
#define TFA9872_SYS_CTRL1_MASK_MANAOOSC		(1<<4)
#define TFA9872_SYS_CTRL1_MASK_MANSCONF		(1<<2)

#define TFA9872_STATUS_FLAGS0				(0x10)
#define TFA9872_STATUS_FLAGS1				(0x11)
#define TFA9872_STATUS_FLAGS3				(0x13)
#define TFA9872_STATUS_FLAGS4				(0x14)
#define TFA9872_STATUS_FLAGS4_MASK_MANSTATE	(0x7<<3)

#define TFA9872_TDM_CONFIG2					(0x22)
#define TFA9872_TDM_CONFIG2_OFFSET_TDMTXUS0	(9)
#define TFA9872_TDM_CONFIG2_MASK_TDMTXUS0	(0x3<<9)

#define TFA9872_INTERRUPT_OUT_REG1			(0x40)
#define TFA9872_INTERRUPT_OUT_REG2			(0x41)
#define TFA9872_INTERRUPT_OUT_REG3			(0x42)

#define TFA9872_INTERRUPT_CLEAR_REG1		(0x44)
#define TFA9872_INTERRUPT_CLEAR_REG2		(0x45)
#define TFA9872_INTERRUPT_CLEAR_REG3		(0x46)

#define TFA9872_INTERRUPT_ENABLE_REG1		(0x48)
#define TFA9872_INTERRUPT_ENABLE_REG2		(0x49)
#define TFA9872_INTERRUPT_ENABLE_REG3		(0x4A)

#define TFA9872_INTERRUPT_MASK_UVP			(1<<4)
#define TFA9872_INTERRUPT_MASK_OVP			(1<<3)
#define TFA9872_INTERRUPT_MASK_UTP			(1<<2)
#define TFA9872_INTERRUPT_MASK_OCP			(1<<9)

#define TFA9872_GAIN_ATT					(0x61)
#define TFA9872_GAIN_ATT_OFFSET_TDMDCG		(6)
#define TFA9872_GAIN_ATT_MASK_TDMDCG		(0xF<<6)
#define TFA9872_GAIN_ATT_OFFSET_TDMSPKG		(2)
#define TFA9872_GAIN_ATT_MASK_TDMSPKG		(0xF<<2)

#define TFA9872_PFM_CONTROL				(0x66)
#define TFA9872_PFM_CONTROL_MASK			(0x1 << 0)

#define TFA9872_DCDC_CONTROL0				(0x70)
#define TFA9872_DCMCC_MASK				(0xf << 3)
#define TFA9872_DCMCC_OFFSET				(0xb << 3)
#define TFA9872_DCDC_CTRL_MASK_DCIE			(1<<9)

#define TFA9872_REG_MAX_NUM					(0xFF)

#endif
