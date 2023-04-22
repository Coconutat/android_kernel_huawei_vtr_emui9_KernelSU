#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include "hisi_hisee.h"
#include "hisi_hisee_fs.h"
#include "hisi_hisee_autotest.h"

/* part 1: channel test function */

int hisee_channel_test_func(void *buf, int para)
{
	int ret = HISEE_OK;
	check_and_print_result();
	set_errno_and_return(ret);
}/*lint !e715*/
