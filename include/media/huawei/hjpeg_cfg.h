/*
 *  Hisilicon K3 SOC camera driver source file
 *
 *  Copyright (C) Huawei Technology Co., Ltd.
 *
 * Author:
 * Email:
 * Date:	  2014-11-15
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


#ifndef __HW_KERNEL_HJPEG_CFG_H__
#define __HW_KERNEL_HJPEG_CFG_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>

typedef enum _encode_format_e
{
    JPGENC_FORMAT_UYVY = 0x0,
    JPGENC_FORMAT_VYUY = 0x1,
    JPGENC_FORMAT_YVYU = 0x2,
    JPGENC_FORMAT_YUYV = 0x3,
    JPGENC_FORMAT_NV12 = 0x10,
    JPGENC_FORMAT_NV21 = 0x11,
}encode_format_e;

/**
 * @brief  
 */
struct ion_handle;

typedef struct _jpgenc_buffer_info_t
{
    encode_format_e                             format;

    unsigned int                                width;
    unsigned int                                height;
    unsigned int                                stride;

    unsigned int                                input_buffer_y;// input iommu addr
    unsigned int                                input_buffer_uv;
    unsigned int                                output_buffer;// output iommu addr
    unsigned int                                quality;
    unsigned int                                rst;
    unsigned int                                output_size;

    int                                         ion_fd;// output ion share_fd
    int                                         input_ion_fd;// input ion share_fd
    union {
        struct ion_handle                       *ion_hdl;
        int64_t                                 _ion_hdl;
    };
    union {
        void                                    *ion_vaddr;
        int64_t                                 _ion_vaddr;
    };

}jpgenc_buffer_info_t;

typedef struct _jpgenc_reg_t
{
    uint32_t                                    addr;
    uint32_t                                    value;
}jpgenc_reg;

typedef struct _jpgenc_config_t
{
    jpgenc_buffer_info_t                        buffer;
    jpgenc_reg                                  reg;
    char                                        filename[256];    
    uint32_t                                    jpegSize;        
}jpgenc_config_t;

/* v4l2 subdev ioctl case id define */
/* #define VIDIOC_HISI_VCM_CFG	_IOWR('V', BASE_VIDIOC_PRIVATE + 31, struct hw_vcm_cfg_data) */
#define HJPEG_ENCODE_PROCESS _IOWR('A', BASE_VIDIOC_PRIVATE + 51,  jpgenc_config_t)
#define HJPEG_ENCODE_POWERON _IO('A', BASE_VIDIOC_PRIVATE + 52)
#define HJPEG_ENCODE_POWERDOWN _IO('A', BASE_VIDIOC_PRIVATE + 53)
#define HJPEG_ENCODE_SETREG  _IOW('A', BASE_VIDIOC_PRIVATE + 54,  jpgenc_config_t)
#define HJPEG_ENCODE_GETREG  _IOWR('A', BASE_VIDIOC_PRIVATE + 55, jpgenc_config_t)

#endif

