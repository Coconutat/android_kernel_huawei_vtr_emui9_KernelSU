/*
 *  Hisilicon K3 SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Author:
 * Email:
 * Date:      2017-01-03
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __HW_KERNEL_FD_DRV_CFG_H__
#define __HW_KERNEL_FD_DRV_CFG_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#ifndef uint32_t
#define uint32_t    u_int32_t
#endif


/**
 Represents the maximum length of fd_reg_array.values array.
 The fd_reg_pair_array.reg_pairs is limited to FD_REG_MAX_ARRAY/2 (because each array element has two uint32_t elements )
*/
#define FD_REG_MAX_ARRAY    81920



/**
Array of registers values at consecutive offsets.
This structure is used as argument for AHFD_WRITE_REGS and AHFD_READ_REGS.

The fd_reg_array.values buffer has the following format:
[offset_1(32bit)] [size_1(32bit) ], size_1 * [reg value(32bit)] , [offset_2(32bit)] [size_2(32bit) ], size_2 * [reg value(32bit)], ... [offset_N(32bit)] [size_N(32bit) ], size_N * [reg value(32bit)]

where fd_reg_array.length = (size_1 + 2) + (size_2 + 2) + ... (size_N + 2)
*/
struct fd_reg_array
{
    uint32_t                                    length;
    uint32_t                                    *values;
    uint32_t                                    img_share_fd;
    uint32_t                                    ipu_share_fd;
};

/**
Type of result AHFD_WAIT_RESULT will wait for.
*/
enum {
    RESULT_TYPE_PRE     = (1<<0),
    RESULT_TYPE_TME     = (1<<1)
};

/**
TME enable mode.
*/
enum {
    MODE_AUTO_TME       = 1,
    MODE_MANUAL_TME     = 2
};

#define AHFD_BASE_IOC  0x90

/**
Retrieves IPU version.
*/
#define AHFD_GET_VERSION          _IOR('v',  AHFD_BASE_IOC, unsigned int)

/**
Writes registers from a fd_reg_array structure.
*/
#define AHFD_WRITE_REGS           _IOW('v',  AHFD_BASE_IOC + 1, struct fd_reg_array)

/**
Reads registers to a fd_reg_array structure.
*/
#define AHFD_READ_REGS            _IOWR('v', AHFD_BASE_IOC + 2, struct fd_reg_array)

/**
Waits for the results of PRE or TME to be ready (it blocks until module(s) finish(es) running).
The result type parameter is given by value.
If given parameter value is RESULT_TYPE_PRE, it waits for PRE to finish.
If given parameter value is RESULT_TYPE_TME, it waits for TME to finish.
If given parameter value is (RESULT_TYPE_PRE | RESULT_TYPE_TME), it waits for both PRE and TME to finish.
*/
#define AHFD_WAIT_RESULT          _IOW('v',  AHFD_BASE_IOC + 3, int)

/**
Sets TME enable mode. It is recommended to call this before enabling PRE.
The mode parameter is given by value.
If given parameter value is MODE_AUTO_TME, TME will be enabled when PRE IRQ arrives.
In this case, it is recommended that TME is configured before starting PRE.
If given parameter value is MODE_MANUAL_TME, TME should be enabled from user space using AHFD_WRITE_REGS* IOCTL.
*/
#define AHFD_SET_MODE             _IOW('v',  AHFD_BASE_IOC + 6, int)

typedef struct _IPU_MAPS_va
{
    unsigned long preImg_va[3];
    unsigned long preMap_va_1[3];
    unsigned long preMap_va_2;
    unsigned long preMap_va_3;
    unsigned long preMap_va_4;
    unsigned long preMap_va_5;
    unsigned long tmeMap_va;
    unsigned long img_size_Y;
    unsigned long img_size_UV;
    uint32_t      img_share_fd;
    uint32_t      ipu_share_fd;
} IO_IPU_MAPS_va;

#define AHFD_SET_MASTER_VA             _IOW('v',  AHFD_BASE_IOC + 7, struct _IPU_MAPS_va)

struct fd_ion_buffer_info
{
    int shared_fd;
    unsigned long phys_addr;
};

#define AHFD_GET_ION_BUFFER            _IOWR('v',  AHFD_BASE_IOC + 8, struct fd_ion_buffer_info)

#endif

