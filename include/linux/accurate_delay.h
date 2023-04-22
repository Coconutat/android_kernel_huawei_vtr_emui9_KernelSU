#ifndef _ACCURATE_DELAY_H_
#define _ACCURATE_DELAY_H_

#ifdef CONFIG_HISI_ACCURATE_DELAY
void accurate_delay_100us(unsigned long us_100);
#else
#include <asm-generic/delay.h>
static void accurate_delay_100us(unsigned long us_100)
{
	udelay(100*us_100);
}
#endif

#endif /* _ACCURATE_DELAY_H_ */
