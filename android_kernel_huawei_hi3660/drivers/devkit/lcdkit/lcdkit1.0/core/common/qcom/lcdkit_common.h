#ifndef __LCDKIT_IO_UTIL_H
#define __LCDKIT_IO_UTIL_H

#ifdef CONFIG_LCDKIT_DRIVER
#define update_value(init, value)  init = value ? value : init;
#define lcdkit_delay(delayvalue)    \
        do { if (delayvalue) { mdelay(delayvalue); } } while (0)
#endif
#endif
