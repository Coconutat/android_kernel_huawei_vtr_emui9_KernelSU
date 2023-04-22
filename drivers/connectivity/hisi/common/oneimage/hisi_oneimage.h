

#ifndef __HISI__ONEIMAGE_H__
#define __HISI__ONEIMAGE_H__

#include <linux/version.h>
#ifdef CONFIG_HWCONNECTIVITY
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0))
#include <linux/huawei/hw_connectivity.h>
#include <linux/huawei/gps/huawei_gps.h>
#else
#include <huawei_platform/connectivity/hw_connectivity.h>
#include <huawei_platform/connectivity/huawei_gps.h>
#endif
#endif

#endif
