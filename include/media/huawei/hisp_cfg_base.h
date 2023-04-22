

#ifndef __HW_KERNEL_HWCAM_HISP_CFG_BASE_H__
#define __HW_KERNEL_HWCAM_HISP_CFG_BASE_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>
#include <linux/compat.h>
#include <linux/iommu.h>

enum {
	HISP_NAME_SIZE = 32,
	HISP_V4L2_EVENT_TYPE = V4L2_EVENT_PRIVATE_START + 0x00090000,

	HISP_HIGH_PRIO_EVENT = 0x1500,
	HISP_MIDDLE_PRIO_EVENT = 0x2000,
	HISP_LOW_PRIO_EVENT = 0x3000,
};

typedef enum _tag_hisp_event_kind {
	HISP_RPMSG_CB,
} hisp_event_kind_t;

typedef struct _tag_hisp_event {
	hisp_event_kind_t kind;
} hisp_event_t;

// daemon  IOMMU define diff with kernel define, follow daemon
// vendor/hisi/ap/bionic/libc/kernel/uapi/media/huawei/mapmodule_cfg.h
// enum
// {
//     POOL_IOMMU_READ    = 1 << 0,
//     POOL_IOMMU_WRITE   = 1 << 1,
//     POOL_IOMMU_EXEC    = 1 << 2,
//     POOL_IOMMU_SEC     = 1 << 3,
//     POOL_IOMMU_CACHE   = 1 << 4,
//     POOL_IOMMU_DEVICE  = 1 << 5,
// };

enum mapType
{
    MAP_TYPE_DYNAMIC = 0,
    MAP_TYPE_RAW2YUV,
    MAP_TYPE_STATIC,
    MAP_TYPE_STATIC_SEC,
    MAP_TYPE_DYNAMIC_CARVEOUT,
    MAP_TYPE_STATIC_ISP_SEC,
    MAP_TYPE_MAX,
};

typedef struct addr_params
{
    uint32_t moduleAddr;
    uint32_t iova;
    uint32_t sharedFd;
    uint32_t type;
    uint32_t prot;
    uint32_t size;
    void *vaddr;
    size_t offset_in_pool;
    size_t pool_align_size;
    uint32_t security_isp_mode;
}addr_param_t;

// enum hisi_isp_rproc_case_attr {
//     DEFAULT_CASE = 0,
//     SEC_CASE,
//     NONSEC_CASE,
//     INVAL_CASE,
// };

struct hisp_cfg_data {
	int cfgtype;
	int mode;
	int isSecure;
	union {
		addr_param_t param;
		uint32_t cfgdata[4];
	};
};

enum hisp_config_type {
	HISP_CONFIG_POWER_ON = 0,
	HISP_CONFIG_POWER_OFF,
    HISP_CONFIG_GET_MAP_ADDR,
    HISP_CONFIG_UNMAP_ADDR,
    HISP_CONFIG_INIT_MEMORY_POOL,
    HISP_CONFIG_DEINIT_MEMORY_POOL,
    HISP_CONFIG_ALLOC_MEM,
    HISP_CONFIG_FREE_MEM,
    HISP_CONFIG_ISP_TURBO,
    HISP_CONFIG_ISP_NORMAL,
    HISP_CONFIG_ISP_LOWPOWER,
    HISP_CONFIG_R8_TURBO,
    HISP_CONFIG_R8_NORMAL,
    HISP_CONFIG_R8_LOWPOWER,
    HISP_CONFIG_PROC_TIMEOUT,
    HISP_CONFIG_MAX_INDEX
};

typedef struct _tag_hisp_info {
	char name[HISP_NAME_SIZE];
} hisp_info_t;

typedef struct _hisp_system_time_t{
    struct timeval s_timeval;
    unsigned int s_system_couter_rate;
    unsigned long long s_system_counter;

}hisp_system_time_t;

#define HISP_IOCTL_POWER_ON   _IO('A', BASE_VIDIOC_PRIVATE + 0x01)
#define HISP_IOCTL_POWER_OFF  _IO('A', BASE_VIDIOC_PRIVATE + 0x02)
#define HISP_IOCTL_GET_INFO   _IOR('A', BASE_VIDIOC_PRIVATE + 0x05, hisp_info_t)
#define HISP_IOCTL_CFG_ISP    _IOWR('A', BASE_VIDIOC_PRIVATE + 0x06, \
				struct hisp_cfg_data)
#define HISP_IOCTL_GET_SYSTEM_TIME   _IOR('A', BASE_VIDIOC_PRIVATE + 0x07, hisp_system_time_t)
#endif /* __HW_KERNEL_HWCAM_HISP_CFG_BASE_H__ */
