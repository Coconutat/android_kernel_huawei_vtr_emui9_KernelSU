
// kernel\include\linux
#ifndef __LINUX_FINGERPRINTHUB_H__
#define __LINUX_FINGERPRINTHUB_H__
#include <linux/ioctl.h>

/* ioctl cmd define */
#define FHBIO                         0xB1

#define FHB_IOCTL_FP_START         _IOW(FHBIO, 0xD1, short)
#define FHB_IOCTL_FP_STOP          _IOW(FHBIO, 0xD2, short)
#define FHB_IOCTL_FP_DISABLE_SET   _IOW(FHBIO, 0xD3, short)

#define FHB_IOCTL_FP_DISABLE_SET_CMD 5

#endif /* __LINUX_FINGERPRINTHUB_H__ */
