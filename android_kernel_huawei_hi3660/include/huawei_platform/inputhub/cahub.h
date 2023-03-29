
// kernel\include\linux
#ifndef __LINUX_CAHUB_H__
#define __LINUX_CAHUB_H__
#include <linux/ioctl.h>

/* ioctl cmd define */
#define CHBIO                         0xB1

#define CHB_IOCTL_CA_START         _IOW(CHBIO, 0xC1, short)
#define CHB_IOCTL_CA_STOP          _IOW(CHBIO, 0xC2, short)
#define CHB_IOCTL_CA_ATTR_START    _IOW(CHBIO, 0xC3, short)
#define CHB_IOCTL_CA_ATTR_STOP     _IOW(CHBIO, 0xC4, short)
#define CHB_IOCTL_CA_INTERVAL_SET  _IOW(CHBIO, 0xC5, short)

/*
  * Warning notes:
  * The below ca values is used by mcu and hal sensorhub module,
  * the value CAHUB_TYPE_XXX below must be consisted with CA_TYPE_XXX
  * of mcu and kernel\drivers\huawei\inputhub\protocol.h.
  */
#define CAHUB_TYPE_PICKUP 0x01
#define CAHUB_TYPE_PUTDOWN 0x02
#define CAHUB_TYPE_ACTIVITY 0x03
#define CAHUB_TYPE_HOLDING 0x04
#define CAHUB_TYPE_MOTION 0x05
#define CAHUB_TYPE_PLACEMENT 0x06
#endif /* __LINUX_CAHUB_H__ */
