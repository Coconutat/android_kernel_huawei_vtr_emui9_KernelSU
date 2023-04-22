#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>


#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mfd/hisi_pmic_mntn.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/reboot.h>
#include <linux/hisi/mmc_trace.h>

#include <linux/version.h>

#include "core.h"
#include "bus.h"
#include "host.h"
#include "sdio_bus.h"

#include "mmc_ops.h"
#include "sd_ops.h"
#include "sdio_ops.h"


extern void mmc_set_ios(struct mmc_host *host);
extern void mmc_bus_get(struct mmc_host *host);
extern void mmc_bus_put(struct mmc_host *host);
extern int mmc_blk_is_retryable(struct mmc_host *mmc);
#ifdef CONFIG_MMC_CQ_HCI
extern void mmc_blk_cmdq_dishalt(struct mmc_card *card);
extern int cmdq_is_reset(struct mmc_host *host);
extern int mmc_blk_cmdq_halt(struct mmc_card *card);
#endif
#ifdef CONFIG_EMMC_FAULT_INJECT
bool g_mmc_reset_status;
#endif

#if defined(CONFIG_HISI_DEBUG_FS)
extern unsigned int sd_test_reset_flag;
#endif

void mmc_power_up_vcc(struct mmc_host *host,u32 ocr)
{
	if (host->ios.power_mode == MMC_POWER_UP)
		return;

	host->ios.vdd = fls(ocr) - 1;

	/*
	 * This delay should be sufficient to allow the power supply
	 * to reach the minimum voltage.
	 */
	mmc_delay(10);

	host->ios.clock = host->ios.clock_store ;
	host->ios.power_mode = MMC_POWER_UP;
	mmc_set_ios(host);

	/*
	 * This delay must be at least 74 clock sizes, or 1 ms, or the
	 * time required to reach a stable voltage.
	 */
	mmc_delay(10);

}


void mmc_power_off_vcc(struct mmc_host *host)
{
	if (host->ios.power_mode == MMC_POWER_OFF)
		return;

	/*store the ios.clock which will be used in mmc_power_up_vcc*/
	host->ios.clock_store = host->ios.clock;
	host->ios.clock = 0;
	host->ios.vdd = 0;

	host->ios.power_mode = MMC_POWER_OFF;

	mmc_set_ios(host);

	/*
	 * Some configurations, such as the 802.11 SDIO card in the OLPC
	 * XO-1.5, require a short delay after poweroff before the card
	 * can be successfully turned on again.
	 */
	mmc_delay(1);
}

void hisi_mmc_power_off(struct mmc_host *host)
{
	if (mmc_card_removable_mmc(host->card))
		mmc_power_off(host);
	else
		mmc_power_off_vcc(host);
}

void hisi_mmc_power_up(struct mmc_host *host)
{
	if (mmc_card_removable_mmc(host->card))
		mmc_power_up(host, host->card->ocr);
	else
		mmc_power_up_vcc(host, host->card->ocr);
}


int mmc_card_awake(struct mmc_host *host)
{
       int err = -ENOSYS;

       mmc_bus_get(host);

       if (host->bus_ops && !host->bus_dead && host->bus_ops->awake)
               err = host->bus_ops->awake(host);

       mmc_bus_put(host);

       return err;
}
EXPORT_SYMBOL(mmc_card_awake);

int mmc_card_sleep(struct mmc_host *host)
{
       int err = -ENOSYS;

       mmc_bus_get(host);

       if (host->bus_ops && !host->bus_dead && host->bus_ops->sleep)
               err = host->bus_ops->sleep(host);

       mmc_bus_put(host);

       return err;
}
EXPORT_SYMBOL(mmc_card_sleep);

int mmc_card_can_sleep(struct mmc_host *host)
{
       struct mmc_card *card = host->card;

       if (card && mmc_card_mmc(card) && card->ext_csd.rev >= 3)
               return 1;
       return 0;
}
EXPORT_SYMBOL(mmc_card_can_sleep);



/*
 * Turn the cache ON/OFF.
 * Turning the cache OFF shall trigger flushing of the data
 * to the non-volatile storage.
 * This function should be called with host claimed
 */
int mmc_cache_ctrl(struct mmc_host *host, u8 enable)
{
       struct mmc_card *card = host->card;
       unsigned int timeout;
       int err = 0;

       if (!(host->caps2 & MMC_CAP2_CACHE_CTRL) ||
                      mmc_card_is_removable(host))
               return err;

       if (card && mmc_card_mmc(card) &&
                       (card->ext_csd.cache_size > 0)) {
               enable = !!enable;

               if (card->ext_csd.cache_ctrl ^ enable) {
                       timeout = enable ? card->ext_csd.generic_cmd6_time : 0;
                       err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
                                       EXT_CSD_CACHE_CTRL, enable, timeout);
                       if (err)
                               pr_err("%s: cache %s error %d\n",
                                               mmc_hostname(card->host),
                                               enable ? "on" : "off",
                                               err);
                       else
                               card->ext_csd.cache_ctrl = enable;
               }
       }

       return err;
}
EXPORT_SYMBOL(mmc_cache_ctrl);

int mmc_card_sleepawake(struct mmc_host *host, int sleep)
{
	struct mmc_command cmd = {0};
	struct mmc_card *card = host->card;
	int err;
	unsigned long timeout;

	if (sleep)
		(void)mmc_deselect_cards(host);

	cmd.opcode = MMC_SLEEP_AWAKE;
	cmd.arg = card->rca << 16;
	if (sleep)
		cmd.arg |= 1 << 15;

	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	err = mmc_wait_for_cmd(host, &cmd, 0);
	if (err)
		return err;

	/*we use the the check busy method to reduce sr time*/
	if (host->ops->card_busy) {
		timeout = jiffies +
			msecs_to_jiffies(DIV_ROUND_UP(card->ext_csd.sa_timeout, 10000));
		while (host->ops->card_busy(host)) {
			/* Timeout if the device never leaves the program state. */
			if (time_after(jiffies, timeout)) {
				pr_err("%s: wait card not busy time out! %s\n",
				mmc_hostname(host), __func__);
				break;
			}
		}
	} else {
		/*
		 * If the host does not wait while the card signals busy, then we will
		 * will have to wait the sleep/awake timeout.  Note, we cannot use the
		 * SEND_STATUS command to poll the status because that command (and most
		 * others) is invalid while the card sleeps.
		 */
		if (!(host->caps & MMC_CAP_WAIT_WHILE_BUSY))
			mmc_delay(DIV_ROUND_UP(card->ext_csd.sa_timeout, 10000));
	}

	if (!sleep)
		err = mmc_select_card(card);

	return err;
}

void mmc_decode_ext_csd_emmc50(struct mmc_card *card, u8 *ext_csd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 76))
	card->ext_csd.pre_eol_info = ext_csd[EXT_CSD_PRE_EOL_INFO];
	card->ext_csd.device_life_time_est_typ_a = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A];
	card->ext_csd.device_life_time_est_typ_b = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B];
#endif
	card->ext_csd.raw_sleep_noti_time = ext_csd[EXT_CSD_SLEEP_NOTIFICATION_TIME];
	if (card->ext_csd.raw_sleep_noti_time > 0 && card->ext_csd.raw_sleep_noti_time <= 0x17)
	{
		card->ext_csd.sleep_notification_time = ((unsigned int)1 << card->ext_csd.raw_sleep_noti_time) / 100;/*ms, raw_sleep_noti_time Units: 10us */
		pr_debug("%s: support SLEEP_NOTIFICATION. sleep_notification_time=%d ms\n", mmc_hostname(card->host), card->ext_csd.sleep_notification_time);
	}

#ifdef CONFIG_MMC_CQ_HCI
	card->ext_csd.cmdq_support = ext_csd[EXT_CSD_CMDQ_SUPPORT] & 0x1;
	if (card->ext_csd.cmdq_support) {
		card->ext_csd.cmdq_depth = (ext_csd[EXT_CSD_CMDQ_DEPTH] & 0x1F) + 1;
		pr_err("%s: %s: CMDQ supported: depth: %d\n",
			mmc_hostname(card->host), __func__,
			card->ext_csd.cmdq_depth);
	}
#endif
}

void mmc_decode_ext_csd_emmc51(struct mmc_card *card, u8 *ext_csd)
{

	/* eMMC5.1*/
	if (card->ext_csd.rev >= 8) {
/*strobe*/
		if (ext_csd[EXT_CSD_STROBE_SUPPORT]) {
			card->ext_csd.strobe_enhanced = ext_csd[EXT_CSD_STROBE_SUPPORT];
			if (card->ext_csd.strobe_enhanced)
				pr_info("%s: support STROBE_ENHANCED.\n", mmc_hostname(card->host));
			else
				pr_warn("%s: EXT_CSD_STROBE_SUPPORT bit is not set\n", mmc_hostname(card->host));
		}
/*cache flush barrier*/
		if (ext_csd[EXT_CSD_BARRIER_SUPPORT]) {
			card->ext_csd.cache_flush_barrier = ext_csd[EXT_CSD_BARRIER_SUPPORT];
			if (card->ext_csd.cache_flush_barrier)
				pr_info("%s: support BARRIER_SUPPORT.\n", mmc_hostname(card->host));
		}
/*cache flush policy*/
		if (ext_csd[EXT_CSD_CACHE_FLUSH_POLICY]) {
			card->ext_csd.cache_flush_policy = ext_csd[EXT_CSD_CACHE_FLUSH_POLICY] & EXT_CSD_CACHE_FLUSH_POLICY_FIFO;
			if (card->ext_csd.cache_flush_policy)
				pr_info("%s: support CACHE_FLUSH_POLICY_FIFO.\n", mmc_hostname(card->host));
		}
/*bkops auto*/
		if (ext_csd[EXT_CSD_BKOPS_SUPPORT] & 0x1) {
			card->ext_csd.bkops_auto_en = ext_csd[EXT_CSD_BKOPS_EN] & EXT_CSD_BKOPS_AUTO_EN;
			if (!card->ext_csd.bkops_auto_en) {
				pr_info("%s: BKOPS_AUTO_EN bit is not set.\n", mmc_hostname(card->host));
			}
		}
/*rpmb 8k*/
		if (ext_csd[EXT_CSD_WR_REL_PARAM] & EXT_CSD_RPMB_REL_WR_EN) {
			pr_info("%s: support RPMB 8K Bytes read/write.\n", mmc_hostname(card->host));
		}
	}

	pr_err("%s: EXT_CSD revision 0x%02x pre_eol_info = 0x%02X device_life_time_est_typ_a = 0x%02X device_life_time_est_typ_b = 0x%02X.\n",
		mmc_hostname(card->host),card->ext_csd.rev, card->ext_csd.pre_eol_info,
		    card->ext_csd.device_life_time_est_typ_a, card->ext_csd.device_life_time_est_typ_b);
}


extern int mmc_screen_test_cache_enable(struct mmc_card *card);
#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
bool hisi_mmc_is_bkops_needed(struct mmc_card *card);
#endif
/*enable feature for emmc5.0 or later contains cmdq,barrier and bkops*/
int mmc_init_card_enable_feature(struct mmc_card *card)
{
	int err = 0;
	struct mmc_host *host = card->host;
    if(!mmc_screen_test_cache_enable(card))
    {
        pr_err("%s:Do emmc screen test, Disable all cache related feature, then return \n ",__func__);
        return err;
    }
	/*
	 * Enable command queue (if supported)
	 */
	if (card->ext_csd.cmdq_support) {
		if (host->caps2 & MMC_CAP2_CMD_QUEUE) {
			card->ext_csd.cmdq_mode_en = 1;
			pr_err("%s: cmdq_mode_en=%d\n", __func__, card->ext_csd.cmdq_mode_en);
		} else {
			card->ext_csd.cmdq_mode_en = 0;
		}
	}

	/*
	 * Enable Barrier feature (if supported)
	 */
	if (card->ext_csd.cache_flush_barrier && (host->caps2 & MMC_CAP2_CACHE_FLUSH_BARRIER)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BARRIER_CTRL, 1,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			return err;
		if (err) {
			pr_warning("%s: Enabling cache flush barrier failed\n",
				   mmc_hostname(card->host));
			err = 0;
			card->ext_csd.cache_flush_barrier_en = 0;
		} else {
			card->ext_csd.cache_flush_barrier_en = 1;
			pr_err("%s: cache_flush_barrier_en=%d\n", __func__, card->ext_csd.cache_flush_barrier_en);
		}
	}

	/*
	 * Enable BKOPS AUTO  feature (if supported)
	 */
	if ((host->caps2 & MMC_CAP2_BKOPS_AUTO_CTRL) && (host->pm_flags & MMC_PM_KEEP_POWER) && card->ext_csd.bkops && (card->ext_csd.rev >= 8)) {
#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
		if (hisi_mmc_is_bkops_needed(card)) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BKOPS_EN, EXT_CSD_BKOPS_MANUAL_EN | EXT_CSD_BKOPS_AUTO_EN,
				card->ext_csd.generic_cmd6_time);
		} else {
#endif
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BKOPS_EN, EXT_CSD_BKOPS_AUTO_EN | (card->ext_csd.man_bkops_en ? EXT_CSD_BKOPS_MANUAL_EN : 0),
				card->ext_csd.generic_cmd6_time);
#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
		}
#endif
		if (err && err != -EBADMSG)
			return err;
		if (err) {
			pr_warning("%s: BKOPS_AUTO_EN failed\n",
				   mmc_hostname(card->host));
			err = 0;
			card->ext_csd.bkops_auto_en = 0;
		} else {
			card->ext_csd.bkops_auto_en = 1;
#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
			if (hisi_mmc_is_bkops_needed(card))
				card->ext_csd.man_bkops_en = 1;
#endif
			pr_err("%s: support BKOPS_AUTO_EN, bkops_auto_en=%d\n", __func__, card->ext_csd.bkops_auto_en);
		}
	}

	return err;
}

/*suspend operation on hisi platform, conclude disable cmdq
 *swich to normal partion, stop bkops and so on
 */
int mmc_suspend_hisi_operation(struct mmc_host *host)
{
	unsigned long timeout = 8000;
	u8 part_config;
	int err = 0;
	if (mmc_card_cmdq(host->card)) {
		/* wait for cmdq req handle done.*/
		while (host->cmdq_ctx.active_reqs) {
			if (timeout == 0) {
				pr_err("%s: wait cmdq complete reqs timeout !\n", __func__);
			    err = -ETIMEDOUT;
				goto err_handle;
			}
			timeout--;
			mdelay(1);
		}

		if (host->cmdq_ops->disable) {
			err = host->cmdq_ops->disable(host, true);
			if (err) {
				pr_err("%s: cmdq disable failed.\n", __func__);
				goto err_handle;
			}
		}

		err = mmc_switch(host->card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_CMDQ_MODE, 0,
			host->card->ext_csd.generic_cmd6_time);

		if (err) {
			pr_err("%s: disable device cmdq failed.active_reqs is %lu, "
				"cmdq status is %d.\n", __func__,
				host->cmdq_ctx.active_reqs, !!mmc_card_cmdq(host->card));
			/*we re-enable the device cmdq feature just in case;
			 *For example:1.rpmb_access disable the device cmdq feature;
			 *2.disable device cmdq feqture failed here in _mmc_suspend;
			 *3.if we don't re-enable device cmdq feature,next normal req
			 *will fail;
			 */
			(void)mmc_switch(host->card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_CMDQ_MODE, 1,
				host->card->ext_csd.generic_cmd6_time);

			if (host->cmdq_ops->enable) {
				if (host->cmdq_ops->enable(host))
					pr_err("%s %d: cmdq enable failed.\n", __func__, __LINE__);
			}
			goto err_handle;
		}

		mmc_card_clr_cmdq(host->card);
	}

	/*
	 * Ensure eMMC user default partition is enabled,because cmd5 will be
	 * a illegal cmd when device is in rpmb partition.
	 */
	if (host->card->ext_csd.part_config & EXT_CSD_PART_CONFIG_ACC_MASK) {
		pr_err("%s need to swich to default partition, current part_config is %d\n",
			__func__, host->card->ext_csd.part_config);
		part_config = host->card->ext_csd.part_config;
		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		err = mmc_switch(host->card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONFIG,
				 part_config,
				 host->card->ext_csd.part_time);
		if (err) {
			pr_err("%s swich to default partition failed, err is %d\n",
				__func__, err);
			goto err_handle;
		}
		host->card->ext_csd.part_config = part_config;
	}

	if (mmc_card_doing_bkops(host->card) && mmc_stop_bkops(host->card))
		err = -1;
err_handle:
	return err;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
int hisi_mmc_reset(struct mmc_host *host)
{
	int ret;
	int retry = 3;
	unsigned long timeout;
	struct mmc_card *card = host->card;

	pr_err("%s enter\n", __func__);
#ifdef CONFIG_EMMC_FAULT_INJECT
	g_mmc_reset_status = true;
#endif

	if (!host->ops->hw_reset)
		return -EOPNOTSUPP;

	do {
		if (host->is_coldboot_on_reset_fail) {
			mmc_set_cold_reset(host);
			timeout = jiffies + 10*60*HZ;
			mod_timer(&host->err_handle_timer, timeout);
		}

		host->ops->hw_reset(host);

		mmc_power_off(host);
		mdelay(200);
		mmc_power_up(host, host->card->ocr);

		ret = host->bus_ops->power_restore(host);
	} while (--retry && ret);

	if (ret) {
		if (host->is_coldboot_on_reset_fail) {
#ifdef CONFIG_HISI_BB
			rdr_syserr_process_for_ap(RDR_MODID_MMC_COLDBOOT, 0, 0);
#else
			machine_restart("AP_S_EMMC_COLDBOOT");
#endif
		} else {
			BUG_ON(1);
		}
	}

	pr_err("%s exit\n", __func__);
#ifdef CONFIG_EMMC_FAULT_INJECT
	g_mmc_reset_status = false;
#endif
	return ret;
}
#pragma GCC diagnostic pop

/**
 * __mmc_send_status_direct - send the cmd 13 to device and get the device
 * status;it can be called when the irq system cannot work.
 */
static int __mmc_send_status_direct(struct mmc_card *card, u32 *status,
				    bool ignore_crc)
{
	int err;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};
	struct mmc_host *host;

	BUG_ON(!card);
	BUG_ON(!card->host);

	host = card->host;
	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	if (ignore_crc)
		cmd.flags &= ~MMC_RSP_CRC;

	cmd.data = NULL;
	cmd.busy_timeout = MMC_TIMEOUT_INVALID;
	mrq.cmd = &cmd;
	memset(cmd.resp, 0, sizeof(cmd.resp));
	cmd.retries = 0;
	mrq.cmd->error = 0;
	mrq.cmd->mrq = &mrq;

	if (host->ops->send_cmd_direct) {
		err = host->ops->send_cmd_direct(host, &mrq);
		if (err)
			return err;
	}

	if (status)
		*status = cmd.resp[0];

	return 0;
}

/**
 * mmc_switch_irq_safe - mmc_switch func for hisi platform;
 * it can be called when the irq system cannot work.
 */
int mmc_switch_irq_safe(struct mmc_card *card, u8 set, u8 index, u8 value)
{
	struct mmc_host *host;
	int err;
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {NULL};
	u32 status = 0;
	u32 cmd_retries = 10;

	BUG_ON(!card);
	BUG_ON(!card->host);
	host = card->host;

	cmd.opcode = MMC_SWITCH;
	cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		  (index << 16) |
		  (value << 8) |
		  set;
	cmd.flags = MMC_CMD_AC;
	cmd.flags |= MMC_RSP_SPI_R1 | MMC_RSP_R1;

	if (index == EXT_CSD_SANITIZE_START)
		cmd.sanitize_busy = true;

	cmd.data = NULL;
	cmd.busy_timeout = MMC_TIMEOUT_INVALID;
	mrq.cmd = &cmd;
	memset(cmd.resp, 0, sizeof(cmd.resp));
	cmd.retries = 0;
	mrq.cmd->error = 0;
	mrq.cmd->mrq = &mrq;

	if (host->ops->send_cmd_direct) {
		/*this func will check busy after data send*/
		err = host->ops->send_cmd_direct(host, &mrq);
		if (err)
			return err;
	}
	do {
		err = __mmc_send_status_direct(card, &status, (bool)false);
		if (err)
			return err;

		/*only check the response status here so we olny send
		 *cmd13 10 times
		 */
		if (--cmd_retries == 0) {
			pr_err("%s: Card stuck in programming state! %s\n",
				mmc_hostname(host), __func__);
			return -ETIMEDOUT;
		}
	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);

	if (status & 0xFDFFA000)
		pr_warn("%s: unexpected status %#x after switch\n",
			mmc_hostname(host), status);
	if (status & R1_SWITCH_ERROR)
		return -EBADMSG;

	return 0;
}

/**
 *	mmc_try_claim_host - try exclusively to claim a host
 *	@host: mmc host to claim
 *
 *	Returns %1 if the host is claimed, %0 otherwise.
 */
int mmc_try_claim_host(struct mmc_host *host)
{
	int claimed_host = 0;
	unsigned long flags;
	bool pm = false;

	if(!spin_trylock_irqsave(&host->lock, flags))/*lint !e666*/
		return 0;

	if (!host->claimed || host->claimer == current) {
		host->claimed = 1;
		host->claimer = current;
		host->claim_cnt += 1;
		claimed_host = 1;
		if (host->claim_cnt == 1)
			pm = true;
	}
	spin_unlock_irqrestore(&host->lock, flags);

	if (pm)
		pm_runtime_get_sync(mmc_dev(host));

	return claimed_host;
}
EXPORT_SYMBOL(mmc_try_claim_host);
/*
 * This is a helper function, which fetches a runtime pm reference for the
 * card device and also claims the host.
 */

int mmc_get_card_hisi(struct mmc_card *card, bool use_irq)
{
	int claimed = 0;
	unsigned int try_count = 200000;
	int ret = 0;

	do {
		claimed = mmc_try_claim_host(card->host);
		if (claimed)
			break;
		udelay(10);
	}while(--try_count>0);
	if (!claimed) {
		pr_err("%s try to claim host failed, claimed:%d, claimer:%s, claim_cnt:%d\n",
			__func__, card->host->claimed,
			card->host->claimer ? card->host->claimer->comm : "NULL",
			card->host->claim_cnt);
		ret = -EIO;
		goto out;
	}

	/*mmc has suspended,no more operation*/
	if (mmc_card_suspended(card)) {
		pr_err("%s mmc has suspended;\n", __func__);
		ret = -EHOSTDOWN;
		goto release_host;
	}

#ifdef CONFIG_MMC_CQ_HCI
	if (card->ext_csd.cmdq_mode_en && cmdq_is_reset(card->host)) {
		pr_err("%s cmdq is in reset process\n", __func__);
		ret = -EHOSTUNREACH;
		goto release_host;
	}
	if (use_irq)
		ret = mmc_blk_cmdq_hangup(card);
	else
		ret = mmc_blk_cmdq_halt(card);

	if (ret) {
		pr_err("%s: cmdq hangup|halt err.\n", __func__);
		ret = -EIO;
		goto release_host;
	}

	if (mmc_card_doing_bkops(card) && mmc_stop_bkops(card))
			pr_err("%s: mmc_stop_bkops failed!\n", __func__);
#endif /* CONFIG_MMC_CQ_HCI */
	return ret;
release_host:
	mmc_release_host(card->host);
out:
	return ret;
}

void mmc_put_card_irq_safe(struct mmc_card *card)
{
#ifdef CONFIG_MMC_CQ_HCI
	mmc_blk_cmdq_dishalt(card);
#endif
	mmc_release_host(card->host);
}


/*
 * Flush the cache to the non-volatile storage.
 */
int mmc_flush_cache_direct(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int err = 0;

	if (!(host->caps2 & MMC_CAP2_CACHE_CTRL))
		return err;

	if (mmc_card_mmc(card) &&
			(card->ext_csd.cache_size > 0) &&
			(card->ext_csd.cache_ctrl & 1)) {

		err = mmc_switch_irq_safe(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_FLUSH_CACHE, 1);
		if (err)
			pr_err("%s: cache flush error %d\n",
					mmc_hostname(card->host), err);
		else
			pr_err("%s success!\n", __func__);
	}

	return err;
}

static void dw_mmc_sd_downshift(struct mmc_card *card)
{
	if(NULL == card)
		return;

	switch(card->sd_bus_speed){
	case UHS_SDR12_BUS_SPEED:
		card->host->caps &= ~MMC_CAP_UHS_SDR12;
		break;
	case UHS_SDR25_BUS_SPEED:
		card->host->caps &= ~MMC_CAP_UHS_SDR25;
		break;
	case UHS_SDR50_BUS_SPEED:
		card->host->caps &= ~MMC_CAP_UHS_SDR50;
		break;
	case UHS_SDR104_BUS_SPEED:
		card->host->caps &= ~MMC_CAP_UHS_SDR104;
		break;
	default:
		break;
	}
}
/*********************sd ops begin**********************/
static int mmc_do_sd_reset(struct mmc_host *host)
{
	int ret = 0;
	struct mmc_card *card = host->card;
#ifdef CONFIG_SD_SDIO_CRC_RETUNING
	/*Only do retuning once,to ensure if the retuning function cant fix the error,we can do reset*/
	if (host->ops->need_retuning && host->bus_ops->mmc_retuning && (0 == host->retuning_count)) {
		if (true == host->ops->need_retuning(host)) {
			host->retuning_count++;
			return host->bus_ops->mmc_retuning(host);
		}
	}
#endif
	/*reset card only once after init card*/
	if (mmc_blk_is_retryable(host)) {
		if (host->reset_count > 3)
			return -EOPNOTSUPP;
	} else {
		if (host->reset_count > 0)
			return -EOPNOTSUPP;
	}

	host->reset_count ++;

             /*make sure if we need slowdown sd clk*/
	if (host->ops->downshift && host->card) {
		if (true == host->ops->downshift(host))
			dw_mmc_sd_downshift(host->card);
	}

	if (!host->bus_ops->power_restore)
		return -EOPNOTSUPP;

	if (!card)
		return -EINVAL;

	/* TODO! Here add hw_reset for ip reset */
	if (host->ops->hw_reset)
		host->ops->hw_reset(host);

	/*clear the reset flag after reset has been done*/
#if defined(CONFIG_HISI_DEBUG_FS)
	sd_test_reset_flag = 0;
#endif
	/* Only for K930/920 SD slow down clk*/
	if (host->ops->slowdown_clk)
		host->ops->slowdown_clk(host, host->ios.timing);

	mmc_power_off(host);
	mmc_set_clock(host, host->f_init);
	/* Wait at least 200 ms */
	mmc_delay(200);
	mmc_power_up(host,host->card->ocr);
	(void)mmc_select_voltage(host, host->card->ocr);

	ret = host->bus_ops->power_restore(host);
	if(0 == ret)
		host->reset_count = 0;
	return ret;
}

#ifdef CONFIG_SD_SDIO_CRC_RETUNING
int mmc_retuning(struct mmc_host *host)
{
	int ret;
	mmc_claim_host(host);

	/*To ensure we set timing to SDR104*/
	mmc_set_timing(host, MMC_TIMING_UHS_SDR12);
	mmc_set_timing(host, MMC_TIMING_UHS_SDR104);

	/*Do tuning*/
	ret =host->ops->execute_tuning(host, MMC_SEND_TUNING_BLOCK);

	pr_err("%s do mmc_retuning\n", __func__);

	mmc_release_host(host);

	return ret;
}
#endif

int mmc_sd_reset(struct mmc_host *host)
{
	return mmc_do_sd_reset(host);
}
EXPORT_SYMBOL(mmc_sd_reset);

/*低速卡，设定频率25M  设置卡位宽*/
void mmc_select_new_sd(struct mmc_card *card)
{
	unsigned int max_dtr;

	mmc_set_timing(card->host, MMC_TIMING_NEW_SD);
	max_dtr = (unsigned int)-1;
	if (max_dtr > card->csd.max_dtr) {
		max_dtr = card->csd.max_dtr;
	}
	mmc_set_clock(card->host, max_dtr);
}



/*********************wifi ops begin**********************/
int mmc_power_save_host_for_wifi(struct mmc_host *host)
{
	int ret = 0;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering down\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if ( (!host->ops || !host->ops->set_ios) ||
         (host->bus_dead) ) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	mmc_bus_put(host);

	mmc_power_off(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_save_host_for_wifi);

int mmc_power_restore_host_for_wifi(struct mmc_host *host)
{
	int ret = 0;

#ifdef CONFIG_MMC_DEBUG
	pr_info("%s: %s: powering up\n", mmc_hostname(host), __func__);
#endif

	mmc_bus_get(host);

	if ( (!host->ops || !host->ops->set_ios) ||
         (host->bus_dead) ) {
		mmc_bus_put(host);
		return -EINVAL;
	}

	mmc_power_up(host,host->card->ocr);

	mmc_bus_put(host);

	return ret;
}
EXPORT_SYMBOL(mmc_power_restore_host_for_wifi);


void mmc_error_handle_timeout_timer(unsigned long data)
{
	unsigned long flags;
	struct mmc_host *host = (struct mmc_host*)data;;

	spin_lock_irqsave(&host->lock, flags);
	host->reset_num = 0;
	hisi_pmic_set_cold_reset(PMIC_HRESET_HOT);
	spin_unlock_irqrestore(&host->lock, flags);
}

void mmc_set_cold_reset(struct mmc_host *host)
{
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);
	hisi_pmic_set_cold_reset(PMIC_HRESET_COLD);
	host->reset_num++;
	spin_unlock_irqrestore(&host->lock, flags);
}
