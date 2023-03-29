
#include "ftsCrossCompile.h"
#include "ftsTime.h"

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdarg.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/time.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
//#include <linux/sec_sysfs.h>

void startStopWatch(StopWatch *w) {
	w->start = current_kernel_time();
}

void stopStopWatch(StopWatch *w) {
	w->end = current_kernel_time();
}

int elapsedMillisecond(StopWatch *w) {
	int result;

	result = ((w->end.tv_sec - w->start.tv_sec)*1000) + (w->end.tv_nsec - w->start.tv_nsec) / 1000000;
	//printf("Elapsed time = %lf ms \n", result);
	return result;
}

int elapsedNanosecond(StopWatch *w) {
	int result;

	result = ((w->end.tv_sec - w->start.tv_sec)*1000000000) + (w->end.tv_nsec - w->start.tv_nsec);
	//printf("Elapsed time = %lf ms \n", result);
	return result;
}

char* timestamp() {
	char* result;
	result = (char *)kmalloc((1)*sizeof(char), GFP_KERNEL);
	if(NULL == result) {
		printk("result alloc failed\n");
		return result;
	}
	result[0] = ' ';
	return result;
}

void stdelay(unsigned long ms)
{
	 mdelay(ms);
}
