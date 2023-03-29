/*
** =============================================================================
** Copyright (c) 2017 Huawei Device Co.Ltd
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
**Author: wangping48@huawei.com
** =============================================================================
*/

#ifndef __SMARTPAKIT_DEFS_H__
#define __SMARTPAKIT_DEFS_H__

#define SMARTPAKIT_NAME_MAX    (64)
#define SMARTPAKIT_NAME_INVALID "none"

// which platform
typedef enum smartpakit_soc_platform {
	SMARTPAKIT_SOC_PLATFORM_HISI = 0, // hisi chip
	SMARTPAKIT_SOC_PLATFORM_QCOM,     // qcom chip
	SMARTPAKIT_SOC_PLATFORM_MTK,

	SMARTPAKIT_SOC_PLATFORM_MAX,
} smartpakit_soc_platform_t;

// pa or smartpa type
// 1. with dsp
// 2. without dsp
//    2.1 use soc dsp
//    2.2 use codec dsp
// 3. simple pa, not smpart pa
//
// algo running on: 0 codec_dsp | 1 soc_dsp | 2 smartpa_dsp
typedef enum smartpakit_algo_in {
	SMARTPAKIT_ALGO_IN_CODEC_DSP = 0,
	SMARTPAKIT_ALGO_IN_SOC_DSP,
	SMARTPAKIT_ALGO_IN_WITH_DSP,
	SMARTPAKIT_ALGO_IN_SIMPLE,
	SMARTPAKIT_ALGO_IN_SIMPLE_WITH_I2C,
	SMARTPAKIT_ALGO_IN_WITH_DSP_PLUGIN, // only use gpio and getinfo

	SMARTPAKIT_ALGO_IN_MAX,
	SMARTPAKIT_ALGO_IN_DSP_MAX = SMARTPAKIT_ALGO_IN_WITH_DSP + 1,
} smartpakit_algo_in_t;

// different device(rec or spk) use different algo params
typedef enum smartpakit_out_device {
	SMARTPAKIT_OUT_DEVICE_SPEAKER = 0,
	SMARTPAKIT_OUT_DEVICE_RECEIVER,

	SMARTPAKIT_OUT_DEVICE_MAX,
} smartpakit_out_device_t;

// out device mask
#define SMARTPAKIT_PA_OUT_DEVICE_MASK      (0xF)
#define SMARTPAKIT_PA_OUT_DEVICE_SHIFT     (4)

// Now, up to only support four pa
typedef enum smartpakit_pa_id {
	SMARTPAKIT_PA_ID_BEGIN = 0,
	SMARTPAKIT_PA_ID_PRIL  = SMARTPAKIT_PA_ID_BEGIN,
	SMARTPAKIT_PA_ID_PRIR,
	SMARTPAKIT_PA_ID_SECL,
	SMARTPAKIT_PA_ID_SECR,

	SMARTPAKIT_PA_ID_MAX,
	SMARTPAKIT_PA_ID_ALL = 0xFF,
} smartpakit_pa_id_t;

// which chip provider
typedef enum smartpakit_chip_vendor {
	SMARTPAKIT_CHIP_VENDOR_MAXIM = 0,    // max98925
	SMARTPAKIT_CHIP_VENDOR_NXP,          // tfa9872, tfa9895
	SMARTPAKIT_CHIP_VENDOR_TI,           // tas2560
	SMARTPAKIT_CHIP_VENDOR_CS,           // cs
	SMARTPAKIT_CHIP_VENDOR_OTHER = SMARTPAKIT_CHIP_VENDOR_CS,  //other
	SMARTPAKIT_CHIP_VENDOR_CUSTOMIZE,    // huawei customize

	SMARTPAKIT_CHIP_VENDOR_MAX,
} smartpakit_chip_vendor_t;

typedef struct smartpakit_info {
	// common info
	unsigned int  soc_platform;
	unsigned int  algo_in;
	unsigned int  out_device;
	unsigned int  pa_num;
	unsigned int  two_in_one;

	// smartpa chip info
	unsigned int  algo_delay_time;
	unsigned int  chip_vendor;
	char chip_model[SMARTPAKIT_NAME_MAX];
} smartpakit_info_t;

typedef struct smartpakit_get_param {
	unsigned int index; // reg addr for smartpa, or gpio index for simple pa
	unsigned int value;
} smartpakit_get_param_t;

// for ioctl cmd SMARTPAKIT_W_ALL
#define SMARTPAKIT_NEED_RESUME_FLAG   (0x10000000)
#define SMARTPAKIT_PA_CTL_MASK        (0x1)
#define SMARTPAKIT_PA_CTL_OFFSET      (4)

// for system/lib64/*.so(64 bits)
typedef struct smartpakit_set_param {
	unsigned int pa_ctl_mask;
	unsigned int param_num;
	unsigned int *params;
} smartpakit_set_param_t;

#define SMARTPAKIT_IO_PARAMS_NUM_MAX   (8 * 1024)  // ioctl params, 8k unsigned int
#define SMARTPAKIT_RW_PARAMS_NUM_MAX   (32 * 1024) // rw params(i2c_transfer), 32k bytes

// for system/lib/*.so(32 bits)
typedef struct smartpakit_set_param_compat {
	unsigned int pa_ctl_mask;
	unsigned int param_num;
	unsigned int params_ptr;
} smartpakit_set_param_compat_t;

// IO controls for smart pa
#define SMARTPAKIT_GET_INFO       _IOR('M', 0x01, smartpakit_info_t)

#define SMARTPAKIT_HW_RESET       _IO( 'M', 0x02) // reset chip
#define SMARTPAKIT_HW_PREPARE     _IO( 'M', 0x03) // prepare chip
#define SMARTPAKIT_HW_UNPREPARE   _IO( 'M', 0x04) // un-prepare chip
#define SMARTPAKIT_REGS_DUMP      _IO( 'M', 0x05) // dump regs list

// read reg cmd
#define SMARTPAKIT_R_PRIL    _IOR('M', 0x11, smartpakit_get_param_t)
#define SMARTPAKIT_R_PRIR    _IOR('M', 0x12, smartpakit_get_param_t)
#define SMARTPAKIT_R_SECL    _IOR('M', 0x13, smartpakit_get_param_t)
#define SMARTPAKIT_R_SECR    _IOR('M', 0x14, smartpakit_get_param_t)

// write regs cmd
#define SMARTPAKIT_INIT      _IOW('M', 0x21, smartpakit_set_param_t)
#define SMARTPAKIT_W_ALL     _IOW('M', 0x22, smartpakit_set_param_t)
#define SMARTPAKIT_W_PRIL    _IOW('M', 0x23, smartpakit_set_param_t)
#define SMARTPAKIT_W_PRIR    _IOW('M', 0x24, smartpakit_set_param_t)
#define SMARTPAKIT_W_SECL    _IOW('M', 0x25, smartpakit_set_param_t)
#define SMARTPAKIT_W_SECR    _IOW('M', 0x26, smartpakit_set_param_t)

// write regs cmd(compat_ioctl)
#define SMARTPAKIT_INIT_COMPAT      _IOW('M', 0x21, smartpakit_set_param_compat_t)
#define SMARTPAKIT_W_ALL_COMPAT     _IOW('M', 0x22, smartpakit_set_param_compat_t)
#define SMARTPAKIT_W_PRIL_COMPAT    _IOW('M', 0x23, smartpakit_set_param_compat_t)
#define SMARTPAKIT_W_PRIR_COMPAT    _IOW('M', 0x24, smartpakit_set_param_compat_t)
#define SMARTPAKIT_W_SECL_COMPAT    _IOW('M', 0x25, smartpakit_set_param_compat_t)
#define SMARTPAKIT_W_SECR_COMPAT    _IOW('M', 0x26, smartpakit_set_param_compat_t)

#endif // __SMARTPAKIT_DEFS_H__

