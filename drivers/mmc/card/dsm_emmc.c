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

#include <linux/mmc/dsm_emmc.h>


unsigned int emmc_dsm_real_upload_size;
static unsigned int g_last_msg_code;	/*last sent error code*/
static unsigned int g_last_msg_count;	/*last sent the code count*/
#define ERR_MAX_COUNT 10

struct dsm_dev dsm_emmc = {
	.name = "dsm_emmc",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = EMMC_DSM_BUFFER_SIZE,
};
struct dsm_client *emmc_dclient;

/*the buffer which transffering to device radar*/
struct emmc_dsm_log g_emmc_dsm_log;

EXPORT_SYMBOL(emmc_dclient);

int can_report(char *max_count)
{
	if (0 == *max_count)
		return 0;
	else {
		(*max_count)--;
		return 1;
	}
}

/*
 * first send the err msg to /dev/exception node.
 * if it produces lots of reduplicated msg, will just record the times
 * for every error, it's better to set max times
 * @code: error number
 * @err_msg: error message
 * @return: 0:don't report, 1: report
 */
static int dsm_emmc_process_log(int code, char *err_msg)
{
	int ret = 0;
	/*the MAX times of erevy err code*/
	static char vdet_err_max_count = ERR_MAX_COUNT;
#ifndef CONFIG_HUAWEI_EMMC_DSM_ONLY_VDET
	static char system_w_err_max_count = ERR_MAX_COUNT;
	static char erase_err_max_count = ERR_MAX_COUNT;
	static char send_cxd_err_max_count = ERR_MAX_COUNT;
	static char emmc_read_err_max_count = ERR_MAX_COUNT;
	static char emmc_write_err_max_count = ERR_MAX_COUNT;
	static char emmc_tuning_err_max_count = ERR_MAX_COUNT;
	static char emmc_set_width_err_max_count = ERR_MAX_COUNT;
	static char emmc_pre_eol_max_count = ERR_MAX_COUNT;
	static char emmc_life_time_err_max_count = ERR_MAX_COUNT;
	static char emmc_rsp_err_max_count = ERR_MAX_COUNT;
	static char emmc_rw_timeout_max_count = ERR_MAX_COUNT;
	static char emmc_host_err_max_count = ERR_MAX_COUNT;
	static char emmc_urgent_bkops_max_count = ERR_MAX_COUNT;
	static char emmc_dyncap_needed_max_count = ERR_MAX_COUNT;
	static char emmc_syspool_exhausted_max_count = ERR_MAX_COUNT;
	static char emmc_packed_failure_max_count = ERR_MAX_COUNT;
	static char emmc_data_crc_failure_max_count = ERR_MAX_COUNT;
	static char emmc_command_crc_failure_max_count = ERR_MAX_COUNT;
#endif
	/*filter: if it has the same msg code with last, record err code&count*/
	if (g_last_msg_code == code) {
		g_last_msg_count++;
		return 0;
	}

	g_last_msg_code = code;
	g_last_msg_count = 0;

	switch (code) {
	case DSM_EMMC_VDET_ERR:
		ret = can_report(&vdet_err_max_count);
		break;
#ifndef CONFIG_HUAWEI_EMMC_DSM_ONLY_VDET
	case DSM_SYSTEM_W_ERR:
		ret = can_report(&system_w_err_max_count);
		break;
	case DSM_EMMC_ERASE_ERR:
		ret = can_report(&erase_err_max_count);
		break;
	case DSM_EMMC_SEND_CXD_ERR:
		ret = can_report(&send_cxd_err_max_count);
		break;
	case DSM_EMMC_READ_ERR:
		ret = can_report(&emmc_read_err_max_count);
		break;
	case DSM_EMMC_WRITE_ERR:
		ret = can_report(&emmc_write_err_max_count);
		break;
	case DSM_EMMC_SET_BUS_WIDTH_ERR:
		ret = can_report(&emmc_set_width_err_max_count);
		break;
	case DSM_EMMC_PRE_EOL_INFO_ERR:
		ret = can_report(&emmc_pre_eol_max_count);
		break;
	case DSM_EMMC_TUNING_ERROR:
		ret = can_report(&emmc_tuning_err_max_count);
		break;
	case DSM_EMMC_LIFE_TIME_EST_ERR:
		ret = can_report(&emmc_life_time_err_max_count);
		break;
	case DSM_EMMC_RSP_ERR:
		ret = can_report(&emmc_rsp_err_max_count);
		break;
	case DSM_EMMC_RW_TIMEOUT_ERR:
		ret = can_report(&emmc_rw_timeout_max_count);
		break;
	case DSM_EMMC_HOST_ERR:
		ret = can_report(&emmc_host_err_max_count);
		break;
	case DSM_EMMC_URGENT_BKOPS:
		ret = can_report(&emmc_urgent_bkops_max_count);
		break;
	case DSM_EMMC_DYNCAP_NEEDED:
		ret = can_report(&emmc_dyncap_needed_max_count);
		break;
	case DSM_EMMC_SYSPOOL_EXHAUSTED:
		ret = can_report(&emmc_syspool_exhausted_max_count);
		break;
	case DSM_EMMC_PACKED_FAILURE:
		ret = can_report(&emmc_packed_failure_max_count);
		break;
	case DSM_EMMC_DATA_CRC:
		ret = can_report(&emmc_data_crc_failure_max_count);
		break;
	case DSM_EMMC_COMMAND_CRC:
		ret = can_report(&emmc_command_crc_failure_max_count);
		break;
#endif
	default:
		ret = 0;
		break;
	}

	return ret;
}

/*
 * Put error message into buffer.
 * @code: error number
 * @err_msg: error message
 * @return: 0:no buffer to report, 1: report
 */
int dsm_emmc_get_log(void *card, int code, char *err_msg)
{
	int ret = 0;
	unsigned int buff_size = sizeof(g_emmc_dsm_log.emmc_dsm_log);
	char *dsm_log_buff = g_emmc_dsm_log.emmc_dsm_log;
	struct mmc_card *card_dev = (struct mmc_card *)card;
	unsigned int last_msg_code = g_last_msg_code;
	unsigned int last_msg_count = g_last_msg_count;
	int i = 1;
	u8 *ext_csd = NULL;

	printk(KERN_ERR "DSM_EMMC enter dsm_emmc_get_log\n");
	if (dsm_emmc_process_log(code, err_msg)) {
		/*clear global buffer*/
		memset(g_emmc_dsm_log.emmc_dsm_log, 0, buff_size);
		/*print duplicated code and its count */
		if ((0 < last_msg_count) && (0 == g_last_msg_count)) {
			ret = snprintf(dsm_log_buff, buff_size, "last Err num: %d, the times: %d\n",
				last_msg_code, last_msg_count + 1);
			if (ret < 0) {
				pr_err("%s:%d ++\n", __func__, __LINE__);
				ret = 0;
			}
			dsm_log_buff += ret;
			buff_size -= (unsigned int)ret;
			last_msg_code = 0;
			last_msg_count = 0;
		}

		printk(KERN_ERR "DSM_EMMC Err num: %d, %s\n", code, err_msg);
		/*print card CID info*/
		if (NULL != card_dev) {
			if (sizeof(struct mmc_cid) < buff_size) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
				/*lint -e559*/
				ret = snprintf(dsm_log_buff, buff_size, "manufacturer_id:%04x Error Num:%lu, %s",
						card_dev->cid.manfid, code, err_msg);
				/*lint +e559*/
#pragma GCC diagnostic pop
				if (ret < 0) {
					pr_err("%s:%d ++\n", __func__, __LINE__);
					ret = 0;
				}
				dsm_log_buff += ret;
				buff_size -= (unsigned int)ret;

				ret = snprintf(dsm_log_buff, buff_size, "product_name:%s ",
						card_dev->cid.prod_name);
				if (ret < 0) {
					pr_err("%s:%d ++\n", __func__, __LINE__);
					ret = 0;
				}
				dsm_log_buff += ret;
				buff_size -= (unsigned int)ret;

				ret = snprintf(dsm_log_buff, buff_size, "fw:%02x%02x%02x%02x%02x%02x%02x%02x ", card_dev->ext_csd.fwrev[0],card_dev->ext_csd.fwrev[1],\
						card_dev->ext_csd.fwrev[2],card_dev->ext_csd.fwrev[3],card_dev->ext_csd.fwrev[4],card_dev->ext_csd.fwrev[5],\
						card_dev->ext_csd.fwrev[6],card_dev->ext_csd.fwrev[7]);

				if (ret < 0) {
					pr_err("%s:%d ++\n", __func__, __LINE__);
					ret = 0;
				}
				dsm_log_buff += ret;
				buff_size -= (unsigned int)ret;

				ret = snprintf(dsm_log_buff, buff_size, "manufacturer_date:%02x\n", (card_dev->raw_cid[3]>>8)&0xff);
				if (ret < 0) {
					pr_err("%s:%d ++\n", __func__, __LINE__);
					ret = 0;
				}
				dsm_log_buff += ret;
				buff_size -= (unsigned int)ret;

			} else {
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}

			/*print card ios info*/
			if (sizeof(card_dev->host->ios) < buff_size) {
				if (NULL != card_dev->host) {
					ret = snprintf(dsm_log_buff, buff_size,
						"Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u, ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
						card_dev->host->ios.clock, card_dev->host->ios.old_rate, card_dev->host->ios.power_mode, card_dev->host->ios.timing,
						card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
					if (ret < 0) {
						pr_err("%s:%d ++\n", __func__, __LINE__);
						ret = 0;
					}
					dsm_log_buff += ret;
					buff_size -= (unsigned int)ret;
					pr_info("Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u, ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
						card_dev->host->ios.clock, card_dev->host->ios.old_rate, card_dev->host->ios.power_mode, card_dev->host->ios.timing,
						card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
				}
			} else {
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}

			/*print card ext_csd info*/
			if (EMMC_EXT_CSD_LENGHT < buff_size) {
				if (NULL != card_dev->cached_ext_csd) {
					ret = snprintf(dsm_log_buff, buff_size,
						"eMMC ext_csd(Note: just static slice data is believable):\n");
					if (ret < 0) {
						pr_err("%s:%d ++\n", __func__, __LINE__);
						ret = 0;
					}
					dsm_log_buff += ret;
					buff_size -= (unsigned int)ret;

					ext_csd = card_dev->cached_ext_csd;
					for (i = 0; i < EMMC_EXT_CSD_LENGHT; i++) {
						ret = snprintf(dsm_log_buff, buff_size, "%02x", ext_csd[i]);
						if (ret < 0) {
							pr_err("%s:%d ++\n", __func__, __LINE__);
							ret = 0;
						}
						dsm_log_buff += ret;
						buff_size -= (unsigned int)ret;
					}
					ret = snprintf(dsm_log_buff, buff_size, "\n\n");
					if (ret < 0) {
						pr_err("%s:%d ++\n", __func__, __LINE__);
						ret = 0;
					}
					dsm_log_buff += ret;
					buff_size -= (unsigned int)ret;
				}
			} else {
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}
		}

		/*get size of used buffer*/
		emmc_dsm_real_upload_size = sizeof(g_emmc_dsm_log.emmc_dsm_log) - buff_size;
		pr_debug("DSM_DEBUG %s\n", g_emmc_dsm_log.emmc_dsm_log);
		printk(KERN_ERR "DSM_EMMC DSM_DEBUG %s\n", g_emmc_dsm_log.emmc_dsm_log);

		return 1;
	} else {
		printk("%s:Err num: %d, %s\n", __FUNCTION__, code, err_msg);
		if (NULL != card_dev) {
			pr_info("Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u,\
				ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
					card_dev->host->ios.clock, card_dev->host->ios.old_rate,
					card_dev->host->ios.power_mode, card_dev->host->ios.timing,
					card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
		}
	}
	printk(KERN_ERR "DSM_EMMC leave dsm_emmc_get_log\n");
	return 0;
}
EXPORT_SYMBOL(dsm_emmc_get_log);

int dsm_emmc_get_life_time(struct mmc_card *card)
{
	int err = 0;

	/* eMMC v5.0 or later */
	if (!strncmp(mmc_hostname(card->host), "mmc0", sizeof("mmc0"))) {
		if (NULL == card->cached_ext_csd) {
			err = mmc_get_ext_csd(card, &card->cached_ext_csd);
			if (err) {
				pr_warn("%s: get ext csd failed err=%d \n",
					mmc_hostname(card->host), err);
				err = 0;
			}
		}
		if (card->ext_csd.rev >= 7) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_EXP_EVENTS_CTRL,
					EXT_CSD_DYNCAP_EVENT_EN,
					card->ext_csd.generic_cmd6_time);
			if (err) {
				pr_warn("%s: Enabling dyncap and syspool event failed\n",
					mmc_hostname(card->host));
				err = 0;
			}

			if (card->ext_csd.device_life_time_est_typ_a >= DEVICE_LIFE_TRIGGER_LEVEL ||
				card->ext_csd.device_life_time_est_typ_b >= DEVICE_LIFE_TRIGGER_LEVEL) {
					DSM_EMMC_LOG(card, DSM_EMMC_LIFE_TIME_EST_ERR,
						"%s:eMMC life time has problem, device_life_time_est_typ_a[268]:%d, device_life_time_est_typ_b{269]:%d\n",
						__FUNCTION__, card->ext_csd.device_life_time_est_typ_a, card->ext_csd.device_life_time_est_typ_b);
			}

			if (card->ext_csd.pre_eol_info == EXT_CSD_PRE_EOL_INFO_WARNING ||
				card->ext_csd.pre_eol_info == EXT_CSD_PRE_EOL_INFO_URGENT) {
				DSM_EMMC_LOG(card, DSM_EMMC_PRE_EOL_INFO_ERR,
					"%s:eMMC average reserved blocks has problem, PRE_EOL_INFO[267]:%d\n", __FUNCTION__,
					card->ext_csd.pre_eol_info);
			}
		}
	}

	return err;
}
EXPORT_SYMBOL(dsm_emmc_get_life_time);

void dsm_emmc_init(void)
{
	if (!emmc_dclient) {
		emmc_dclient = dsm_register_client(&dsm_emmc);
	}
	mutex_init(&g_emmc_dsm_log.lock);
}
EXPORT_SYMBOL(dsm_emmc_init);
