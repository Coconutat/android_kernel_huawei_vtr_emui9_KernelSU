#include <linux/kref.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/hidraw.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/tty.h>
#include <linux/crc16.h>
#include <linux/crc-ccitt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <huawei_platform/log/log_exception.h>

#include "sw_debug.h"
#include <linux/device.h>
#include <linux/version.h>
//#include <linux/sw_interface.h>

#define SW_DEBUG_BUFSIZE 512


void sw_debug_dump_data(const u8 *data,int count)
{
	int i =0;
	if(count < 1)
		return;

	SW_PRINT_DBG("[sw_Report]Len %d\n",count);
	SW_PRINT_DBG("{ ");
	for(i = 0; i< count;i++)
	{
		SW_PRINT_DBG( "%x ", data[i]);
		if( (i>0) && (i/10==0))
		{
			SW_PRINT_DBG("\n");
		}
	}
	SW_PRINT_DBG("} \n");
}




