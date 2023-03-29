#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <linux/uaccess.h>

#include "queue.h"

#include <linux/mmc/hw_write_protect.h>
#include <linux/scatterlist.h>

#include <linux/mmc/dsm_sdcard.h>

struct dsm_dev dsm_sdcard = {
	.name = "dsm_sdcard",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};
struct dsm_client *sdcard_dclient;
char g_dsm_log_sum[1024] = {0};

EXPORT_SYMBOL(sdcard_dclient);

struct dsm_sdcard_cmd_log dsm_sdcard_cmd_logs[] = {
	{"CMD8 : ",				0},
	{"CMD55 : ",    		0},
	{"ACMD41: ",			0},
	{"CMD2_RESP0 : ",		0},
	{"CMD2_RESP1 : ",		0},
	{"CMD2_RESP2 : ",		0},
	{"CMD2_RESP3 : ",		0},
	{"CMD3 : ",				0},
	{"CMD9_RESP0 : ",		0},
	{"CMD9_RESP1 : ",		0},
	{"CMD9_RESP2 : ",		0},
	{"CMD9_RESP3 : ",		0},
	{"CMD7 : ",				0},
	{"CMD6_CMDERR : ",      0},
	{"CMD6_DATERR : ",      0},
	{"BLK_STUCK_IN_PRG_ERR : ",      0},
	{"BLK_WR_SPEED_ERR : ",      0},
	{"BLK_RW_CHECK_ERR : ",      0},
	{"CSD_REG_RO_ERR : ",      0},
	{"FILESYSTEM_ERR : ",      0},
	{"LOWSPEED_SPEC_ERR : ",      0},
	{"Report Uevent : ",	0},
	{"HARDWARE_TIMEOUT_ERR : ",	0},
	{"MMC_BLK_ABORT : ",	0},
	{"MMC_BLK_ECC_ERR : ",	0},
	{"CMD13_CMDERR : ",     0},
	{"CMD13_DATAERR : ",    0},
	{"ACMD51_CMDERR : ",    0},
	{"ACMD51_DATAERR : ",   0},

        {/*sentinel*/}
};

char *dsm_sdcard_get_log(int cmd, int err)
{
	int i;
	int ret = 0;
	int buff_size = sizeof(g_dsm_log_sum);
	char *dsm_log_buff = g_dsm_log_sum;

	memset(g_dsm_log_sum, 0, buff_size);

	ret = snprintf(dsm_log_buff, buff_size, "Err : %d\n", err);
	dsm_log_buff += ret;
	buff_size -= ret;

	for (i = 0; i <= cmd; i++) {
		ret = snprintf(dsm_log_buff, buff_size,
						"%s%08x\n", dsm_sdcard_cmd_logs[i].log,
						dsm_sdcard_cmd_logs[i].value);/* [false alarm]*/
		if (ret > buff_size - 1) {
			printk(KERN_ERR "Buff size is not enough\n");
			printk(KERN_ERR "%s", g_dsm_log_sum);
			return g_dsm_log_sum;
		}

		dsm_log_buff += ret;
		buff_size -= ret;
	}

	pr_debug("DSM_DEBUG %s", g_dsm_log_sum);

	return g_dsm_log_sum;
}
EXPORT_SYMBOL(dsm_sdcard_get_log);

void dsm_sdcard_report(int cmd, int err)
{
	int	buff_len = 0;
	char *log_buff = NULL;

	if (!dsm_client_ocuppy(sdcard_dclient)) {
		log_buff = dsm_sdcard_get_log(cmd, 0);
		buff_len = strlen(log_buff);
		dsm_client_copy(sdcard_dclient, log_buff, buff_len + 1);
		dsm_client_notify(sdcard_dclient, err);
	}
}
EXPORT_SYMBOL(dsm_sdcard_report);

void dsm_sdcard_init(void)
{
	if (!sdcard_dclient) {
		sdcard_dclient = dsm_register_client(&dsm_sdcard);
	}
}
EXPORT_SYMBOL(dsm_sdcard_init);
