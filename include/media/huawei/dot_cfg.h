/*
 *  Hisilicon K3 SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
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

#ifndef __HW_ALAN_KERNEL_HWCAM_DOT_CFG_H__
#define __HW_ALAN_KERNEL_HWCAM_DOT_CFG_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>

typedef enum _tag_hwdot_config_type
{
    HWCAM_DOT_POWERON,
    HWCAM_DOT_POWEROFF,
//TODO...
} hwdot_config_type_t;

enum
{
    HWDOT_NAME_SIZE                          =   32,
    HWDOT_V4L2_EVENT_TYPE                    =   V4L2_EVENT_PRIVATE_START + 0x00080000,

    HWDOT_HIGH_PRIO_EVENT                    =   0x1500,
    HWDOT_MIDDLE_PRIO_EVENT                  =   0x2000,
    HWDOT_LOW_PRIO_EVENT                     =   0x3000,
};

typedef struct _tag_hwdot_config_data
{
    uint32_t cfgtype;
}hwdot_config_data_t;

typedef struct _tag_hwdot_info
{
    char           name[HWDOT_NAME_SIZE];
    int            i2c_idx;
} hwdot_info_t;

typedef enum _tag_hwdot_event_kind
{
    HWDOT_INFO_ERROR,
} hwdot_event_kind_t;

typedef struct _tag_hwdot_event
{
    hwdot_event_kind_t                          kind;
    union // can ONLY place 10 int fields here.
    {
        struct
        {
            uint32_t                            id;
        }error;
    }data;
}hwdot_event_t;

typedef struct _dot_thermal_data{
    int data;
}dot_thermal_data;

#define HWDOT_IOCTL_GET_INFO                 _IOR('T',  BASE_VIDIOC_PRIVATE + 17, hwdot_info_t)
#define HWDOT_IOCTL_CONFIG                  _IOWR('T',  BASE_VIDIOC_PRIVATE + 19, hwdot_config_data_t)
#define HWDOT_IOCTL_GET_THERMAL           _IOR('T',  BASE_VIDIOC_PRIVATE + 20, dot_thermal_data)

#endif // __HW_ALAN_KERNEL_HWCAM_DOT_CFG_H__
 
