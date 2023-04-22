#ifndef __HISI_FIQ_H
#define __HISI_FIQ_H
#include <soc_rtctimerwdt_interface.h>

#define WDTLOCK         SOC_RTCTIMERWDT_WDLOCK_ADDR(0)
#define WDTINTCLR       SOC_RTCTIMERWDT_WDINTCLR_ADDR(0)
#define WDTLOAD         SOC_RTCTIMERWDT_WDLOAD_ADDR(0)
#define WDTCONTROL      SOC_RTCTIMERWDT_WDCONTROL_ADDR(0)
#define UNLOCK      0x1ACCE551
#define LOCK        0x00000001
#define WDT_ENABlE (0x3)
#define WDT_DISABLE (0x0)
#define WDT_TIMEOUT_KICK (32768*3)

extern void __iomem	*wdt_base;
/*
* Call a function on IPI_MNTN_INFORM
*/
#ifdef CONFIG_HISI_FIQ
extern void hisi_mntn_inform(void);
#else
static inline void hisi_mntn_inform(void){}
#endif

#endif
