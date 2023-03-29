

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
#include "mmc_hisi_card.h"
#include <linux/mmc/hw_write_protect.h>
#include <linux/scatterlist.h>

#define WP_TEMP      "temporary write protection"
#define WP_POWERON   "power-on write protection"
#define WP_PERMANENT "permanent write protection"

/* get write protection block info */
static int do_get_write_protection(struct gendisk *disk, struct hd_struct *part)
{
	struct mmc_card *card;
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	void *data_buf;
	int len = 8;
	unsigned char buf[8], temp_char, wp_flag;
	unsigned int sector_start_addr, wp_group_size;
	char line_buf[128];
	int i, j, ret = 0;

	/* make sure this is a main partition*/
	card = mmc_get_card_by_disk(disk);
	if (IS_ERR(card)) {
		ret = PTR_ERR(card);
		return ret;
	}

	sector_start_addr = part->start_sect;
	wp_group_size = (512 * 1024) *
		card->ext_csd.raw_hc_erase_gap_size *
		card->ext_csd.raw_hc_erase_grp_size / 512;
	pr_info("[INFO] %s: sector_start_addr = 0x%x. wp_group_size = 0x%x.\n",
			__func__, sector_start_addr, wp_group_size);
	data_buf = kzalloc(len, GFP_KERNEL);
	if (!data_buf) {
		pr_err("Malloc err at %d.\n", __LINE__);
		return -ENOMEM;
	}
	mrq.cmd = &cmd;
	mrq.data = &data;
	cmd.opcode = MMC_SEND_WRITE_PROT_TYPE;
	cmd.arg = sector_start_addr;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = len;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, data_buf, len);
	mmc_get_card(card);
	mmc_set_data_timeout(&data, card);
	mmc_wait_for_req(card->host, &mrq);
	mmc_put_card(card);

	memcpy(buf, data_buf, len);

	for (i = 7; i > 0; i--) {
		temp_char = buf[i];
		for (j = 0; j < 4; j++) {
			wp_flag = temp_char & 0x3;
			snprintf(line_buf, 128 - 1,
					"[0x%08x~0x%08x] Write protection group is ",
					sector_start_addr,
					sector_start_addr + wp_group_size - 1);
			sector_start_addr += wp_group_size;
			temp_char = temp_char >> 2;
			switch (wp_flag) {
			case 0:
				strncat(line_buf, "disable", strlen("disable"));
				break;
			case 1:
				strncat(line_buf, WP_TEMP,
						strlen(WP_TEMP));
				break;
			case 2:
				strncat(line_buf, WP_POWERON,
						strlen(WP_POWERON));
				break;
			case 3:
				strncat(line_buf, WP_PERMANENT,
						strlen(WP_PERMANENT));
				break;
			}
			pr_err("%s: %s\n", mmc_hostname(card->host), line_buf);
		}
	}

	if (cmd.error) {
		ret = 1;
		pr_err("cmd.error=%d\n", cmd.error);
		goto out;
	}
	if (data.error) {
		ret = 1;
		pr_err("data.error=%d\n", data.error);
		goto out;
	}
out:
	kfree(data_buf);
	return ret;
}

static int mmc_wp_condition_check_for_part(struct mmc_card *card,
		struct hd_struct *part)
{
	unsigned int sector_start, sector_size, wp_group_size;

	sector_start = (unsigned int)(part->start_sect);
	sector_size  = (unsigned int)(part->nr_sects);

	/*  check whether the parttion sector size is
	 *  aligned with wp_group_size, calculating the
	 *  loop count for sending SET_WRITE_PROTECT (CMD28) */
	wp_group_size = (512 * 1024) *
		card->ext_csd.raw_hc_erase_gap_size *
		card->ext_csd.raw_hc_erase_grp_size / 512;
	if (sector_size % wp_group_size) {
		pr_err("%s: Write protected areas need to be aligned in accordance with wp_group_size.\n",
				mmc_hostname(card->host));
		return -EINVAL;
	}
	pr_info("%s: Adrr is aligned with wp_gpoup_size.\n",
			mmc_hostname(card->host));

	/* check whether sector start addr is aligned with wp_group_size*/
	if (sector_start % wp_group_size) {
		pr_err("addr sector_start is not aligned %d %d.\n",
				sector_start, wp_group_size);
	} else {
		pr_info("addr sector_start is aliged.\n");
	}

	pr_info("write protection, part %s, start 0x%x,length 0x%x, group 0x%x\n",
			part->info->volname, sector_start,
			sector_size, wp_group_size);

	return 0;

}

static int mmc_wp_enable_hardware_reset(struct mmc_card *card)
{
	int err;

	if (0 == mmc_can_reset(card)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_RST_N_FUNCTION, EXT_CSD_RST_N_ENABLED,
				card->ext_csd.generic_cmd6_time);
		if (err) {
			pr_err("%s: set hardware reset fail\n",
					mmc_hostname(card->host));
			return -1;
		}
	}
	return 0;
}

static int mmc_wp_prepare_for_emmc(struct mmc_card *card)
{
	int err = 0;
	int retries = 3;
	unsigned char tmp;
	u8 *ext_csd;

	mmc_get_card(card);

	err = mmc_get_ext_csd(card, &ext_csd);
	if (err) {
		pr_err("%s: err %d sending ext_csd.\n",
				mmc_hostname(card->host), err);
		goto out;
	}

	/* to enable hardware reset, in case no poweroff on reboot */
	if (mmc_wp_enable_hardware_reset(card)) {
		err = -1;
		goto out;
	}

	if ((ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x01) == 0) {
		pr_err("EXT_CSD_ERASE_GROUP_DEF in ext_csd  was set failed before\n");
		err = -1;
		goto out;
	}
	pr_info("ext_csd erase_group_def is 0x%x.\n",
			ext_csd[EXT_CSD_ERASE_GROUP_DEF]);
	pr_info("INFO mmc_switch before, ext_csd.user_wp is 0x%x.\n",
			ext_csd[EXT_CSD_USER_WP]);
	while (!(ext_csd[EXT_CSD_USER_WP] & EXT_CSD_BOOT_WP_B_PWR_WP_EN)) {
		if (retries-- == 0) {
			err = -1;
			goto out;
		}

		/*
		 * US_PERM_WP_EN   US_PWR_WP_EN   Type of protection set
		 *                                by SET_WRITE_PROT command
		 *             0              0   Temporary
		 *             0              1   Power-On
		 *             1              0   Permanent
		 *             1              1   Permanent
		 */
		tmp = ext_csd[EXT_CSD_USER_WP];
		tmp |= EXT_CSD_BOOT_WP_B_PWR_WP_EN;
		tmp &= ~EXT_CSD_BOOT_WP_B_PERM_WP_EN;
		tmp |= EXT_CSD_BOOT_WP_B_PERM_WP_DIS;
		tmp |= EXT_CSD_BOOT_WP_B_PWR_WP_DIS;

		/* enable power-on bit */
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_USER_WP, tmp,
				card->ext_csd.generic_cmd6_time);
		if (err) {
			pr_err("%s: set write protect 0x%02x failed.\n",
					mmc_hostname(card->host), tmp);
		}

		err = mmc_get_ext_csd(card, &ext_csd);
		if (err) {
			pr_err("%s: mmc_get_ext_csd failed.\n",
					mmc_hostname(card->host));
		}

		pr_info("mmc_switch end, ext_csd.user_wp is 0x%x.\n",
				ext_csd[EXT_CSD_USER_WP]);
	}

out:
	mmc_put_card(card);
	return err;
}

static int get_card_status(struct mmc_card *card, u32 *status, int retries)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, retries);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

static int mmc_wp_start(struct mmc_card *card, struct hd_struct *part)
{
	unsigned int sector_start, sector_size, wp_group_size;
	unsigned int loop_count, status;
	struct mmc_command cmd = {0};
	int err = 0;
	unsigned int i = 0;

	sector_start = (unsigned int)(part->start_sect);
	sector_size  = (unsigned int)(part->nr_sects);
	wp_group_size = (512 * 1024) * card->ext_csd.raw_hc_erase_gap_size *
		card->ext_csd.raw_hc_erase_grp_size / 512;
	loop_count = sector_size / wp_group_size;

	mmc_get_card(card);
	cmd.opcode = MMC_SET_WRITE_PROT;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;

	for (i = 0; i < loop_count; i++) {
		/* Sending CMD28 for each WP group size */
		cmd.arg = sector_start + i * wp_group_size;
		err = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (err)
			goto out;

		/* Sending CMD13 to check card status */
		do {
			err = get_card_status(card, &status, 3);
			if (R1_CURRENT_STATE(status) == R1_STATE_TRAN)
				break;
		} while ((!err) && (R1_CURRENT_STATE(status) == R1_STATE_PRG));
		if (err)
			goto out;
	}

	pr_info("%s: sucessed.have protect num =%d,total loop= %d\n",
			__func__, i, loop_count);
out:
	mmc_put_card(card);
	return err;

}

static int do_set_write_protection(struct gendisk *disk, struct hd_struct *part)
{
	struct mmc_card *card;
	int err = 0;

	card = mmc_get_card_by_disk(disk);
	if (IS_ERR(card)) {
		return PTR_ERR(card);
	}

	err = mmc_wp_condition_check_for_part(card, part);
	if (err)
		return err;

	err = mmc_wp_prepare_for_emmc(card);
	if (err)
		goto out;

	err = mmc_wp_start(card, part);
	if (err)
		goto out;
out:
	return err;
}

static int part_wp_action(struct block_device *bdev, const char *partname,
		int (*func)(struct gendisk *disk, struct hd_struct *part))
{
	struct gendisk *sgp = bdev->bd_disk;
	struct disk_part_iter piter;
	struct hd_struct *part;
	int ret = 0;

	/* Don't show non-partitionable
	 * removable devices or empty devices */
	if (!get_capacity(sgp) || (!disk_max_parts(sgp) &&
				(sgp->flags & GENHD_FL_REMOVABLE)))
		return -1;
	if (sgp->flags & GENHD_FL_SUPPRESS_PARTITION_INFO)
		return -1;

	/* show the full disk and all non-0 size partitions of it */
	disk_part_iter_init(&piter, sgp, DISK_PITER_INCL_PART0);
	while ((part = disk_part_iter_next(&piter))) {
		if (part->info && part->info->volname[0] &&
				!strncmp(part->info->volname,
					partname, strlen(partname))) {
			pr_info("SYS_WP: on partition: %s\n", partname);
			ret = func(sgp, part);
			break;
		}
	}
	disk_part_iter_exit(&piter);
	return ret;
}

int mmc_hw_set_wp_state(struct block_device *bdev)
{
	int ret;

	ret  = part_wp_action(bdev, "system", do_set_write_protection);
	ret |= part_wp_action(bdev, "cust",   do_set_write_protection);

	return ret;
}
EXPORT_SYMBOL(mmc_hw_set_wp_state);

int mmc_hw_get_wp_state(struct block_device *bdev)
{
	int ret;

	ret  = part_wp_action(bdev, "system", do_get_write_protection);
	ret |= part_wp_action(bdev, "cust",   do_get_write_protection);

	return ret;
}
EXPORT_SYMBOL(mmc_hw_get_wp_state);
