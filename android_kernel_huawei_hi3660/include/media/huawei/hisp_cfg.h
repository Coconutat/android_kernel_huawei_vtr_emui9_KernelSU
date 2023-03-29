

#ifndef __HW_JACKY_KERNEL_HWCAM_HISP_CFG_H__
#define __HW_JACKY_KERNEL_HWCAM_HISP_CFG_H__

#include "hisp_msg.h"
#include "hisp_cfg_base.h"

#define HISP_IOCTL_SEND_RPMSG _IOW('A', BASE_VIDIOC_PRIVATE + 0x03, hisp_msg_t)
#define HISP_IOCTL_RECV_RPMSG _IOR('A', BASE_VIDIOC_PRIVATE + 0x04, hisp_msg_t)

#endif /* __HW_JACKY_KERNEL_HWCAM_HISP_CFG_H__ */
