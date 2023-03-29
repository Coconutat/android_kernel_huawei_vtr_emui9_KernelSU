/*
 *  include/linux/usb/rt1711.h
 *  Include header file for Richtek RT1711 TypeC Port Only Driver
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Jeff Chang <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_RT1711_H
#define __LINUX_RT1711_H
#include <huawei_platform/usb/pd/richtek/pd_dbg_info.h>

/*show debug message or not */
#define ENABLE_RT1711_DBG	0

#define RT1711_REG_VENDOR_ID		(0x00)
#define RT1711_REG_ALERT		(0x10)
#define RT1711_REG_ALERT_MASK		(0x12)
#define RT1711_REG_POWER_STATUS_MASK	(0x14)
#define RT1711_REG_CCSTATUS		(0x18)
#define RT1711_REG_POWER_STATUS		(0x19)
#define RT1711_REG_ROLECTRL		(0x1a)
#define RT1711_REG_POWERCTRL		(0x1c)
#define RT1711_REG_MSGHEADERINFO	(0x2e)
#define RT1711_REG_RCVDETECT		(0x2f)
#define RT1711_REG_RCVBYTECNT		(0x30)
#define RT1711_REG_RXBUFFRAMETYPE	(0x31)
#define RT1711_REG_RXBUFHEADER		(0x32)
#define RT1711_REG_RXDATA		(0x34)
#define RT1711_REG_TRANSMITHEADERLOWCMD	(0x42)
#define RT1711_REG_TRANSMIT		(0x50)
#define RT1711_REG_TRANSMITBYTECNT	(0x51)
#define RT1711_REG_TXHDR		(0x52)
#define RT1711_REG_TXDATA		(0x54)
#define RT1711_REG_PHYCTRL3		(0x82)
#define RT1711_REG_PHYCTRL6		(0x85)
#define RT1711_REG_RXTXDBG		(0x8c)
#define RT1711_REG_LOW_POWER_CTRL	(0x90)
#define RT1711_REG_CC_EXT_CTRL		(0x9F)
#define RT1711_REG_SWRESET		(0xa0)
#define RT1711_REG_TTCPCFILTER		(0xa1)
#define RT1711_REG_DRPTOGGLECYCLE	(0xa2)
#define RT1711_REG_DRPDUTYCTRL		(0xa3)

#define RT1711_CC1STAT_MASK		(0x03)
#define RT1711_CC1STAT_SHIFT		(0)
#define RT1711_CC2STAT_MASK		(0x0c)
#define RT1711_CC2STAT_SHIFT		(2)
#define RT1711_DRPRESULT_MASK		(0x10)
#define RT1711_DRPRESULT_SHIFT		(4)
#define RT1711_DRPSTAT_MASK		(0x20)
#define RT1711_DRPSTAT_SHIFT		(5)
#define RT1711_CC1ROLE_MASK		(0x03)
#define RT1711_CC1ROLE_SHIFT		(0)
#define RT1711_CC2ROLE_MASK		(0x0c)
#define RT1711_CC2ROLE_SHIFT		(2)
#define RT1711_RPVALUE_MASK		(0x30)
#define RT1711_RPVALUE_SHIFT		(4)
#define RT1711_DRPROLE_MASK		(0xc0)
#define RT1711_DRPROLE_SHIFT		(6)

#define RT1711_DRP_TOGGLING_MASK	(0x20)
#define RT1711_VBUS_SAFE0V_MASK		(0x40)
#define RT1711_VBUS_PRES_MASK		(0x20)
#define RT1711_ROLECTRL_DRP_MASK	(0x40)
#define RT1711_PLUG_ORIENT_MASK		(0x10)
#define RT1711_VCONN_MASK		(0x01)
#define RT1711_CC_CHANGED_MASK		(0x01)
#define RT1711_PS_CHANGED_MASK		(0x02)
#define RT1711_ALERT_RXSTAT_MASK	(0x04)
#define RT1711_RXTX_DBG_TX_BUSY		(0x40)

/*
 * RT1711_REG_ALERT			(0x10)
 * RT1711_REG_ALERT_MASK		(0x12)
 */

#define RT1711_REG_ALERT_WAKEUP			(1<<7)
#define RT1711_REG_ALERT_TX_SUCCESS	(1<<6)
#define RT1711_REG_ALERT_TX_DISCARDED	(1<<5)
#define RT1711_REG_ALERT_TX_FAILED	(1<<4)
#define RT1711_REG_ALERT_RX_HARD_RST	(1<<3)
#define RT1711_REG_ALERT_RX_STATUS	(1<<2)
#define RT1711_REG_ALERT_POWER_STATUS	(1<<1)
#define RT1711_REG_ALERT_CC_STATUS		(1<<0)

/*
 * RT1711_REG_RXTXDBG			(0x8c)
 */

#define RT1711_REG_RXTXDBG_RX_BUSY		(1<<7)
#define RT1711_REG_RXTXDBG_TX_BUSY		(1<<6)

/*
 * RT1711_REG_LOW_POWER_CTRL		(0x90)
 */

#define RT1711_DISCHARGE_EN				(1<<5)
#define RT1711_BMCIO_LPRPRD				(1<<4)
#define RT1711_BMCIO_LPEN				(1<<3)
#define RT1711_BMCIO_BG_EN				(1<<2)
#define RT1711_REG_VBUS_DETEN			(1<<1)
#define RT1711_REG_BMCIO_OSC_EN			(1<<0)

/*
 * RT1711_REG_CC_EXT_CTRL			(0x9F)
 */

#define RT1711_WAKEUP_EN				(1<<7)
#define RT1711_CK_40K_SEL				(1<<6)
#define RT1711_RA_DETACH_STATUS			(1<<5)
#define RT1711_RA_DETACH_MASK			(1<<4)

#if ENABLE_RT1711_DBG
#define RT1711_INFO(format, args...) \
	pd_dbg_info("%s() line-%d: " format,\
	__func__, __LINE__, ##args)
#else
#define RT1711_INFO(foramt, args...)
#endif

#endif /* #ifndef __LINUX_RT1711_H */
