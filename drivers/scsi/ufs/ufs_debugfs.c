/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * UFS debugfs - add debugfs interface to the ufshcd.
 * This is currently used for statistics collection and exporting from the
 * UFS driver.
 * This infrastructure can be used for debugging or direct tweaking
 * of the driver from userspace.
 *
 */

#define pr_fmt(fmt) "ufshcd :" fmt

#include <linux/random.h>
#include "ufs_debugfs.h"
#include "unipro.h"
#include "ufshcd.h"
#include "ufshci.h"

enum field_width {
	BYTE = 1,
	WORD = 2,
};

struct desc_field_offset {
	char *name;
	int offset;
	enum field_width width_byte;
};

static int ufshcd_tag_req_type(struct request *rq)
{
	int rq_type = TS_WRITE;

	if (!rq || !(rq->cmd_type & REQ_TYPE_FS))
		rq_type = TS_NOT_SUPPORTED;
	else if (rq->cmd_flags & REQ_PREFLUSH)
		rq_type = TS_FLUSH;
	else if (rq_data_dir(rq) == READ)
		rq_type = (rq->cmd_flags & REQ_URGENT) ?
			TS_URGENT_READ : TS_READ;
	else if (rq->cmd_flags & REQ_URGENT)
		rq_type = TS_URGENT_WRITE;

	return rq_type;
}

void ufshcd_update_error_stats(struct ufs_hba *hba, int type)
{
	if (type < UFS_ERR_MAX)
		hba->ufs_stats.err_stats[type]++;
}

void ufshcd_update_tag_stats(struct ufs_hba *hba, int tag)
{
	struct request *rq =
		hba->lrb[tag].cmd ? hba->lrb[tag].cmd->request : NULL;
	u64 **tag_stats = hba->ufs_stats.tag_stats;
	int rq_type;

	if (!hba->ufs_stats.enabled)
		return;

	tag_stats[tag][TS_TAG]++;
	if (!rq || !(rq->cmd_type & REQ_TYPE_FS))
		return;

	WARN_ON(hba->ufs_stats.q_depth > hba->nutrs);
	rq_type = ufshcd_tag_req_type(rq);
	if (!(rq_type < 0 || rq_type > TS_NUM_STATS))
		tag_stats[hba->ufs_stats.q_depth++][rq_type]++;
}

void ufshcd_update_tag_stats_completion(
	struct ufs_hba *hba, struct scsi_cmnd *cmd)
{
	struct request *rq = cmd ? cmd->request : NULL;

	if (rq && rq->cmd_type & REQ_TYPE_FS)
		hba->ufs_stats.q_depth--;
}

void update_req_stats(struct ufs_hba *hba, struct ufshcd_lrb *lrbp)
{
	int rq_type;
	struct request *rq = lrbp->cmd ? lrbp->cmd->request : NULL;
	s64 delta = lrbp->complete_time_stamp - lrbp->issue_time_stamp;

	/* update general request statistics */
	if (hba->ufs_stats.req_stats[TS_TAG].count == 0)
		hba->ufs_stats.req_stats[TS_TAG].min = delta;
	hba->ufs_stats.req_stats[TS_TAG].count++;
	hba->ufs_stats.req_stats[TS_TAG].sum += delta;
	if (delta > hba->ufs_stats.req_stats[TS_TAG].max) /*lint !e574*/
		hba->ufs_stats.req_stats[TS_TAG].max = delta;
	if (delta < hba->ufs_stats.req_stats[TS_TAG].min) /*lint !e574*/
			hba->ufs_stats.req_stats[TS_TAG].min = delta;

	rq_type = ufshcd_tag_req_type(rq);
	if (rq_type == TS_NOT_SUPPORTED)
		return;

	/* update request type specific statistics */
	if (hba->ufs_stats.req_stats[rq_type].count == 0)
		hba->ufs_stats.req_stats[rq_type].min = delta;
	hba->ufs_stats.req_stats[rq_type].count++;
	hba->ufs_stats.req_stats[rq_type].sum += delta;
	if (delta > hba->ufs_stats.req_stats[rq_type].max) /*lint !e574*/
		hba->ufs_stats.req_stats[rq_type].max = delta;
	if (delta < hba->ufs_stats.req_stats[rq_type].min) /*lint !e574*/
			hba->ufs_stats.req_stats[rq_type].min = delta;
}

#define UFS_ERR_STATS_PRINT(file, error_index, string, error_seen)	\
	do {								\
		if (err_stats[error_index]) {				\
			seq_printf(file, string,			\
					err_stats[error_index]);	\
			error_seen = true;				\
		}							\
	} while (0)

#define DOORBELL_CLR_TOUT_US	(1000 * 1000)	/* 1 sec */

#define BUFF_LINE_SIZE 16	/* Must be a multiplication of sizeof(u32) */
#define TAB_CHARS 8

static int ufsdbg_tag_stats_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	struct ufs_stats *ufs_stats;
	int i, j;
	int max_depth;
	bool is_tag_empty = true;
	unsigned long flags;
	char *sep = " | * | ";

	if (!hba)
		goto exit;

	ufs_stats = &hba->ufs_stats;

	if (!ufs_stats->enabled) {
		pr_debug("%s: ufs statistics are disabled\n", __func__);
		seq_puts(file, "ufs statistics are disabled\n");
		goto exit;
	}

	max_depth = hba->nutrs;

	spin_lock_irqsave(hba->host->host_lock, flags);
	/* Header */
	seq_printf(file,
		   " Tag Stat\t\t%s Number of pending reqs upon issue (Q fullness)\n",
		   sep);
	for (i = 0; i < TAB_CHARS * (TS_NUM_STATS + 4); i++) {
		seq_puts(file, "-");
		if (i == (TAB_CHARS * 3 - 1))
			seq_puts(file, sep);
	}
	seq_printf(file,
		   "\n #\tnum uses\t%s\t #\tAll\tRead\tWrite\tUrg.R\tUrg.W\tFlush\n",
		   sep);

	/* values */
	for (i = 0; i < max_depth; i++) {
		if (ufs_stats->tag_stats[i][TS_TAG] <= 0 &&
		    ufs_stats->tag_stats[i][TS_READ] <= 0 &&
		    ufs_stats->tag_stats[i][TS_WRITE] <= 0 &&
		    ufs_stats->tag_stats[i][TS_URGENT_READ] <= 0 &&
		    ufs_stats->tag_stats[i][TS_URGENT_WRITE] <= 0 &&
		    ufs_stats->tag_stats[i][TS_FLUSH] <= 0)
			continue;

		is_tag_empty = false;
		seq_printf(file, " %d\t ", i);
		for (j = 0; j < TS_NUM_STATS; j++) {
			seq_printf(file, "%llu\t", ufs_stats->tag_stats[i][j]);
			if (j != 0)
				continue;
			seq_printf(file, "\t%s\t %d\t%llu\t", sep, i,
				   ufs_stats->tag_stats[i][TS_READ] +
				   ufs_stats->tag_stats[i][TS_WRITE] +
				   ufs_stats->tag_stats[i][TS_URGENT_READ] +
				   ufs_stats->tag_stats[i][TS_URGENT_WRITE] +
				   ufs_stats->tag_stats[i][TS_FLUSH]);
		}
		seq_puts(file, "\n");
	}
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	if (is_tag_empty)
		pr_debug("%s: All tags statistics are empty", __func__);

exit:
	return 0;
}

static int ufsdbg_tag_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_tag_stats_show, inode->i_private);
}

static ssize_t ufsdbg_tag_stats_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	struct ufs_stats *ufs_stats;
	int val = 0;
	int ret, bit = 0;
	unsigned long flags;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret) {
		dev_err(hba->dev, "%s: Invalid argument\n", __func__);
		return ret;
	}

	ufs_stats = &hba->ufs_stats;
	spin_lock_irqsave(hba->host->host_lock, flags);

	if (!val) {
		ufs_stats->enabled = false;
		pr_debug("%s: Disabling UFS tag statistics", __func__);
	} else {
		ufs_stats->enabled = true;
		pr_debug("%s: Enabling & Resetting UFS tag statistics",
			 __func__);
		memset(hba->ufs_stats.tag_stats[0], 0,
		       sizeof(**hba->ufs_stats.tag_stats) *
		       TS_NUM_STATS * hba->nutrs);

		/* initialize current queue depth */
		ufs_stats->q_depth = 0;
		for_each_set_bit_from(bit, &hba->outstanding_reqs, hba->nutrs)
		    ufs_stats->q_depth++;
		pr_debug("%s: Enabled UFS tag statistics", __func__);
	}

	spin_unlock_irqrestore(hba->host->host_lock, flags);
	return cnt;
}

static const struct file_operations ufsdbg_tag_stats_fops = {
	.open = ufsdbg_tag_stats_open,
	.read = seq_read,
	.write = ufsdbg_tag_stats_write,
};

static int ufsdbg_err_stats_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	int *err_stats;
	unsigned long flags;
	bool error_seen = false;

	if (!hba)
		goto exit;

	err_stats = hba->ufs_stats.err_stats;

	spin_lock_irqsave(hba->host->host_lock, flags);

	seq_puts(file, "\n==UFS errors that caused controller reset==\n");

	UFS_ERR_STATS_PRINT(file, UFS_ERR_HIBERN8_EXIT,
			    "controller reset due to hibern8 exit error:\t %d\n",
			    error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_VOPS_SUSPEND,
			    "controller reset due to vops suspend error:\t\t %d\n",
			    error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_EH,
			    "controller reset due to error handling:\t\t %d\n",
			    error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_CLEAR_PEND_XFER_TM,
			    "controller reset due to clear xfer/tm regs:\t\t %d\n",
			    error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_INT_FATAL_ERRORS,
			    "controller reset due to fatal interrupt:\t %d\n",
			    error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_INT_UIC_ERROR,
			    "controller reset due to uic interrupt error:\t %d\n",
			    error_seen);

	if (error_seen)
		error_seen = false;
	else
		seq_puts(file,
			 "so far, no errors that caused controller reset\n\n");

	seq_puts(file, "\n\n==UFS other errors==\n");

	UFS_ERR_STATS_PRINT(file, UFS_ERR_HIBERN8_ENTER,
			    "hibern8 enter:\t\t %d\n", error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_RESUME,
			    "resume error:\t\t %d\n", error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_SUSPEND,
			    "suspend error:\t\t %d\n", error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_LINKSTARTUP,
			    "linkstartup error:\t\t %d\n", error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_POWER_MODE_CHANGE,
			    "power change error:\t %d\n", error_seen);

	UFS_ERR_STATS_PRINT(file, UFS_ERR_TASK_ABORT,
			    "abort callback:\t\t %d\n\n", error_seen);

	if (!error_seen)
		seq_puts(file, "so far, no other UFS related errors\n\n");

	spin_unlock_irqrestore(hba->host->host_lock, flags);
exit:
	return 0;
}

static int ufsdbg_err_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_err_stats_show, inode->i_private);
}

static ssize_t ufsdbg_err_stats_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	struct ufs_stats *ufs_stats;
	unsigned long flags;

	ufs_stats = &hba->ufs_stats;
	spin_lock_irqsave(hba->host->host_lock, flags);

	pr_debug("%s: Resetting UFS error statistics", __func__);
	memset(ufs_stats->err_stats, 0, sizeof(hba->ufs_stats.err_stats));

	spin_unlock_irqrestore(hba->host->host_lock, flags);
	return cnt;
}

static const struct file_operations ufsdbg_err_stats_fops = {
	.open = ufsdbg_err_stats_open,
	.read = seq_read,
	.write = ufsdbg_err_stats_write,
};

static int ufshcd_init_statistics(struct ufs_hba *hba)
{
	struct ufs_stats *stats = &hba->ufs_stats;
	int ret = 0;
	int i;

	stats->enabled = false;
	stats->tag_stats = kzalloc(sizeof(*stats->tag_stats) * hba->nutrs,
				   GFP_KERNEL);
	if (!hba->ufs_stats.tag_stats)
		goto no_mem;

	stats->tag_stats[0] = kzalloc(sizeof(**stats->tag_stats) *
				      TS_NUM_STATS * hba->nutrs, GFP_KERNEL);
	if (!stats->tag_stats[0])
		goto no_mem;

	for (i = 1; i < hba->nutrs; i++)
		stats->tag_stats[i] = &stats->tag_stats[0][i * TS_NUM_STATS];/*lint !e679*/

	memset(stats->err_stats, 0, sizeof(hba->ufs_stats.err_stats));

	goto exit;

no_mem:
	dev_err(hba->dev, "%s: Unable to allocate UFS tag_stats", __func__);
	ret = -ENOMEM;
exit:
	return ret;
}

static void ufsdbg_pr_buf_to_std(struct ufs_hba *hba, int offset, int num_regs,
			  char *str, void *priv)
{
	int i;
	char linebuf[38];
	int size = num_regs * sizeof(u32);
	int lines = size / BUFF_LINE_SIZE + (size % BUFF_LINE_SIZE ? 1 : 0);
	struct seq_file *file = priv;

	if (!hba || !file) {
		pr_err("%s called with NULL pointer\n", __func__);
		return;
	}

	for (i = 0; i < lines; i++) {
		hex_dump_to_buffer(hba->mmio_base + offset + i * BUFF_LINE_SIZE, /*lint !e679*/
				   min(BUFF_LINE_SIZE, size), BUFF_LINE_SIZE, 4,
				   linebuf, sizeof(linebuf), false);
		seq_printf(file, "%s [%x]: %s\n", str, i * BUFF_LINE_SIZE,
			   linebuf);
		size -= BUFF_LINE_SIZE / sizeof(u32);
	}
}

static int ufsdbg_host_regs_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;

	pm_runtime_get_sync(hba->dev);
	ufsdbg_pr_buf_to_std(hba, 0, REG_SPACE_SIZE / sizeof(u32),
			     "host regs", file);
	pm_runtime_put_sync(hba->dev);
	return 0;
}

static int ufsdbg_host_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_host_regs_show, inode->i_private);
}

static const struct file_operations ufsdbg_host_regs_fops = {
	.open = ufsdbg_host_regs_open,
	.read = seq_read,
};

static int ufsdbg_dump_device_desc_show(struct seq_file *file, void *data)
{
	int err = 0;
	int buff_len = QUERY_DESC_DEVICE_MAX_SIZE;
	u8 desc_buf[QUERY_DESC_DEVICE_MAX_SIZE];
	struct ufs_hba *hba = (struct ufs_hba *)file->private;

	struct desc_field_offset device_desc_field_name[] = {
		{"bLength", 0x00, BYTE},
		{"bDescriptorType", 0x01, BYTE},
		{"bDevice", 0x02, BYTE},
		{"bDeviceClass", 0x03, BYTE},
		{"bDeviceSubClass", 0x04, BYTE},
		{"bProtocol", 0x05, BYTE},
		{"bNumberLU", 0x06, BYTE},
		{"bNumberWLU", 0x07, BYTE},
		{"bBootEnable", 0x08, BYTE},
		{"bDescrAccessEn", 0x09, BYTE},
		{"bInitPowerMode", 0x0A, BYTE},
		{"bHighPriorityLUN", 0x0B, BYTE},
		{"bSecureRemovalType", 0x0C, BYTE},
		{"bSecurityLU", 0x0D, BYTE},
		{"Reserved", 0x0E, BYTE},
		{"bInitActiveICCLevel", 0x0F, BYTE},
		{"wSpecVersion", 0x10, WORD},
		{"wManufactureDate", 0x12, WORD},
		{"iManufactureName", 0x14, BYTE},
		{"iProductName", 0x15, BYTE},
		{"iSerialNumber", 0x16, BYTE},
		{"iOemID", 0x17, BYTE},
		{"wManufactureID", 0x18, WORD},
		{"bUD0BaseOffset", 0x1A, BYTE},
		{"bUDConfigPLength", 0x1B, BYTE},
		{"bDeviceRTTCap", 0x1C, BYTE},
		{"wPeriodicRTCUpdate", 0x1D, WORD}
	};

	pm_runtime_get_sync(hba->dev);
	err = ufshcd_read_device_desc(hba, desc_buf, buff_len);
	pm_runtime_put_sync(hba->dev);

	if (!err) {
		unsigned int i;
		struct desc_field_offset *tmp;
		for (i = 0; i < ARRAY_SIZE(device_desc_field_name); ++i) {
			tmp = &device_desc_field_name[i];

			if (tmp->width_byte == BYTE) {
				seq_printf(file,
					   "Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
					   tmp->offset,
					   tmp->name,
					   (u8) desc_buf[tmp->offset]);
			} else if (tmp->width_byte == WORD) {
				seq_printf(file,
					   "Device Descriptor[Byte offset 0x%x]: %s = 0x%x\n",
					   tmp->offset,
					   tmp->name,
					   *(u16 *)&desc_buf[tmp->offset]);
			} else {
				seq_printf(file,
					   "Device Descriptor[offset 0x%x]: %s. Wrong Width = %d",
					   tmp->offset, tmp->name,
					   tmp->width_byte);
			}
		}
	} else {
		seq_printf(file, "Reading Device Descriptor failed. err = %d\n",
			   err);
	}

	return err;
}

static int ufsdbg_show_hba_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;

	seq_printf(file, "hba->outstanding_tasks = 0x%x\n",
		   (u32) hba->outstanding_tasks);
	seq_printf(file, "hba->outstanding_reqs = 0x%x\n",
		   (u32) hba->outstanding_reqs);

	seq_printf(file, "hba->capabilities = 0x%x\n", hba->capabilities);
	seq_printf(file, "hba->nutrs = %d\n", hba->nutrs);
	seq_printf(file, "hba->nutmrs = %d\n", hba->nutmrs);
	seq_printf(file, "hba->ufs_version = 0x%x\n", hba->ufs_version);
	seq_printf(file, "hba->irq = 0x%x\n", hba->irq);
	seq_printf(file, "hba->auto_bkops_enabled = %d\n",
		   hba->auto_bkops_enabled);

	seq_printf(file, "hba->ufshcd_state = 0x%x\n", hba->ufshcd_state);
	seq_printf(file, "hba->eh_flags = 0x%x\n", hba->eh_flags);
	seq_printf(file, "hba->intr_mask = 0x%x\n", hba->intr_mask);
	seq_printf(file, "hba->ee_ctrl_mask = 0x%x\n", hba->ee_ctrl_mask);

	/* HBA Errors */
	seq_printf(file, "hba->errors = 0x%x\n", hba->errors);
	seq_printf(file, "hba->uic_error = 0x%x\n", hba->uic_error);
	seq_printf(file, "hba->saved_err = 0x%x\n", hba->saved_err);
	seq_printf(file, "hba->saved_uic_err = 0x%x\n", hba->saved_uic_err);

	hba->vops->dbg_hci_dump(hba);
	hba->vops->dbg_uic_dump(hba);

	return 0;
}

static int ufsdbg_show_hba_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_show_hba_show, inode->i_private);
}

static const struct file_operations ufsdbg_show_hba_fops = {
	.open = ufsdbg_show_hba_open,
	.read = seq_read,
};

static int ufsdbg_dump_device_desc_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			   ufsdbg_dump_device_desc_show, inode->i_private);
}

static const struct file_operations ufsdbg_dump_device_desc = {
	.open = ufsdbg_dump_device_desc_open,
	.read = seq_read,
};

static int ufsdbg_power_mode_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	char *names[] = {
		"INVALID MODE",
		"FAST MODE",
		"SLOW MODE",
		"INVALID MODE",
		"FASTAUTO MODE",
		"SLOWAUTO MODE",
		"INVALID MODE",
	};

	/* Print current status */
	seq_puts(file, "UFS current power mode [RX, TX]:");
	seq_printf(file, "gear=[%d,%d], lane=[%d,%d], pwr=[%s,%s], rate = %c",
		   hba->pwr_info.gear_rx, hba->pwr_info.gear_tx,
		   hba->pwr_info.lane_rx, hba->pwr_info.lane_tx,
		   names[hba->pwr_info.pwr_rx],
		   names[hba->pwr_info.pwr_tx],
		   hba->pwr_info.hs_rate == PA_HS_MODE_B ? 'B' : 'A');
	seq_puts(file, "\n\n");

	/* Print usage */
	seq_puts(file,
		 "To change power mode write 'GGLLMM' where:\n"
		 "G - selected gear\n"
		 "L - number of lanes\n"
		 "M - power mode:\n"
		 "\t1 = fast mode\n"
		 "\t2 = slow mode\n"
		 "\t4 = fast-auto mode\n"
		 "\t5 = slow-auto mode\n"
		 "first letter is for RX, second letter is for TX.\n\n");

	return 0;
}

static bool ufsdbg_power_mode_validate(struct ufs_pa_layer_attr *pwr_mode)
{
	if (pwr_mode->gear_rx < UFS_PWM_G1 || pwr_mode->gear_rx > UFS_PWM_G7 ||
	    pwr_mode->gear_tx < UFS_PWM_G1 || pwr_mode->gear_tx > UFS_PWM_G7 ||
	    pwr_mode->lane_rx < 1 || pwr_mode->lane_rx > 2 ||
	    pwr_mode->lane_tx < 1 || pwr_mode->lane_tx > 2 ||
	    (pwr_mode->pwr_rx != FAST_MODE && pwr_mode->pwr_rx != SLOW_MODE &&
	     pwr_mode->pwr_rx != FASTAUTO_MODE &&
	     pwr_mode->pwr_rx != SLOWAUTO_MODE) ||
	    (pwr_mode->pwr_tx != FAST_MODE && pwr_mode->pwr_tx != SLOW_MODE &&
	     pwr_mode->pwr_tx != FASTAUTO_MODE &&
	     pwr_mode->pwr_tx != SLOWAUTO_MODE)) {
		pr_err("%s: power parameters are not valid\n", __func__);
		return false;
	}

	return true;
}

static int ufsdbg_cfg_pwr_param(struct ufs_hba *hba,
				struct ufs_pa_layer_attr *new_pwr,
				struct ufs_pa_layer_attr *final_pwr)
{
	int ret = 0;
	bool is_dev_sup_hs = false;
	bool is_new_pwr_hs = false;
	int dev_pwm_max_rx_gear;
	int dev_pwm_max_tx_gear;

	if (!hba->max_pwr_info.is_valid) {
		dev_err(hba->dev,
			"%s: device max power is not valid. can't configure power\n",
			__func__);
		return -EINVAL;
	}
	if (hba->max_pwr_info.info.pwr_rx == FAST_MODE ||
			hba->max_pwr_info.info.pwr_rx == FASTAUTO_MODE)
		is_dev_sup_hs = true;

	if (new_pwr->pwr_rx == FAST_MODE || new_pwr->pwr_rx == FASTAUTO_MODE)
		is_new_pwr_hs = true;

	final_pwr->lane_rx = hba->max_pwr_info.info.lane_rx;
	final_pwr->lane_tx = hba->max_pwr_info.info.lane_tx;

	/* device doesn't support HS but requested power is HS */
	if (!is_dev_sup_hs && is_new_pwr_hs) {
		pr_err("%s: device doesn't support HS. requested power is HS\n",
		       __func__);
		return -ENOTSUPP;
	} else if ((is_dev_sup_hs && is_new_pwr_hs) ||
		   (!is_dev_sup_hs && !is_new_pwr_hs)) {
		/*
		 * If device and requested power mode are both HS or both PWM
		 * then dev_max->gear_xx are the gears to be assign to
		 * final_pwr->gear_xx
		 */
		final_pwr->gear_rx = hba->max_pwr_info.info.gear_rx;
		final_pwr->gear_tx = hba->max_pwr_info.info.gear_tx;
	} else if (is_dev_sup_hs && !is_new_pwr_hs) {
		/*
		 * If device supports HS but requested power is PWM, then we
		 * need to find out what is the max gear in PWM the device
		 * supports
		 */

		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_MAXRXPWMGEAR),
			       &dev_pwm_max_rx_gear);

		if (!dev_pwm_max_rx_gear) {
			pr_err("%s: couldn't get device max pwm rx gear\n",
			       __func__);
			ret = -EINVAL;
			goto out;
		}

		ufshcd_dme_peer_get(hba, UIC_ARG_MIB(PA_MAXRXPWMGEAR),
				    &dev_pwm_max_tx_gear);

		if (!dev_pwm_max_tx_gear) {
			pr_err("%s: couldn't get device max pwm tx gear\n",
			       __func__);
			ret = -EINVAL;
			goto out;
		}

		final_pwr->gear_rx = dev_pwm_max_rx_gear;
		final_pwr->gear_tx = dev_pwm_max_tx_gear;
	}

	if ((new_pwr->gear_rx > final_pwr->gear_rx) ||
	    (new_pwr->gear_tx > final_pwr->gear_tx) ||
	    (new_pwr->lane_rx > final_pwr->lane_rx) ||
	    (new_pwr->lane_tx > final_pwr->lane_tx)) {
		pr_err
		    ("%s: (RX,TX) GG,LL: in PWM/HS new pwr [%d%d,%d%d]exceeds device limitation [%d%d,%d%d]\n",
		     __func__, new_pwr->gear_rx, new_pwr->gear_tx,
		     new_pwr->lane_rx, new_pwr->lane_tx, final_pwr->gear_rx,
		     final_pwr->gear_tx, final_pwr->lane_rx,
		     final_pwr->lane_tx);
		return -ENOTSUPP;
	}

	final_pwr->gear_rx = new_pwr->gear_rx;
	final_pwr->gear_tx = new_pwr->gear_tx;
	final_pwr->lane_rx = new_pwr->lane_rx;
	final_pwr->lane_tx = new_pwr->lane_tx;
	final_pwr->pwr_rx = new_pwr->pwr_rx;
	final_pwr->pwr_tx = new_pwr->pwr_tx;
	final_pwr->hs_rate = new_pwr->hs_rate;

out:
	return ret;
}

static int ufsdbg_config_pwr_mode(struct ufs_hba *hba,
				  struct ufs_pa_layer_attr *desired_pwr_mode)
{
	int ret;

	pm_runtime_get_sync(hba->dev);
	scsi_block_requests(hba->host);
	ret = ufshcd_wait_for_doorbell_clr(hba, DOORBELL_CLR_TOUT_US);
	if (!ret)
		ret = ufshcd_change_power_mode(hba, desired_pwr_mode);
	scsi_unblock_requests(hba->host);
	pm_runtime_put_sync(hba->dev);

	return ret;
}

static ssize_t ufsdbg_power_mode_write(struct file *file,
				       const char __user *ubuf, size_t cnt,
				       loff_t *ppos)
{
	struct ufs_hba *hba = file->f_mapping->host->i_private;
	struct ufs_pa_layer_attr pwr_mode;
	struct ufs_pa_layer_attr final_pwr_mode;
	char pwr_mode_str[BUFF_LINE_SIZE] = { 0 };
	loff_t buff_pos = 0;
	int ret;
	int idx = 0;

	(void) simple_write_to_buffer(pwr_mode_str, BUFF_LINE_SIZE,
				     &buff_pos, ubuf, cnt);

	pwr_mode.gear_rx = pwr_mode_str[idx++] - '0';
	pwr_mode.gear_tx = pwr_mode_str[idx++] - '0';
	pwr_mode.lane_rx = pwr_mode_str[idx++] - '0';
	pwr_mode.lane_tx = pwr_mode_str[idx++] - '0';
	pwr_mode.pwr_rx = pwr_mode_str[idx++] - '0';
	pwr_mode.pwr_tx = pwr_mode_str[idx++] - '0';

	/*
	 * Switching between rates is not currently supported so use the
	 * current rate.
	 * TODO: add rate switching if and when it is supported in the future
	 */
	pwr_mode.hs_rate = hba->pwr_info.hs_rate;

	/* Validate user input */
	if (!ufsdbg_power_mode_validate(&pwr_mode))
		return -EINVAL;

	pr_debug
	    ("%s: new power mode requested [RX,TX]: Gear=[%d,%d], Lane=[%d,%d], Mode=[%d,%d]\n",
	     __func__, pwr_mode.gear_rx, pwr_mode.gear_tx, pwr_mode.lane_rx,
	     pwr_mode.lane_tx, pwr_mode.pwr_rx, pwr_mode.pwr_tx);

	ret = ufsdbg_cfg_pwr_param(hba, &pwr_mode, &final_pwr_mode);
	if (ret) {
		dev_err(hba->dev,
			"%s: failed to configure new power parameters, ret = %d\n",
			__func__, ret);
		return cnt;
	}

	ret = ufsdbg_config_pwr_mode(hba, &final_pwr_mode);
	if (ret == -EBUSY)
		dev_err(hba->dev,
			"%s: ufshcd_config_pwr_mode failed: system is busy, try again\n",
			__func__);
	else if (ret)
		dev_err(hba->dev,
			"%s: ufshcd_config_pwr_mode failed, ret=%d\n",
			__func__, ret);

	return cnt;
}

static int ufsdbg_power_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_power_mode_show, inode->i_private);
}

static const struct file_operations ufsdbg_power_mode_desc = {
	.open = ufsdbg_power_mode_open,
	.read = seq_read,
	.write = ufsdbg_power_mode_write,
};

static int ufsdbg_dme_read(void *data, u64 *attr_val, bool peer)
{
	int ret;
	struct ufs_hba *hba = data;
	u32 attr_id, read_val = 0;
	int (*read_func) (struct ufs_hba *, u32, u32 *);

	if (!hba)
		return -EINVAL;

	read_func = peer ? ufshcd_dme_peer_get : ufshcd_dme_get;
	attr_id = peer ? hba->debugfs_files.dme_peer_attr_id :
	    hba->debugfs_files.dme_local_attr_id;
	pm_runtime_get_sync(hba->dev);
	scsi_block_requests(hba->host);
	ret = ufshcd_wait_for_doorbell_clr(hba, DOORBELL_CLR_TOUT_US);
	if (!ret)
		ret = read_func(hba, UIC_ARG_MIB(attr_id), &read_val);
	scsi_unblock_requests(hba->host);
	pm_runtime_put_sync(hba->dev);

	if (!ret)
		*attr_val = (u64) read_val;

	return ret;
}

static int ufsdbg_dme_local_set_attr_id(void *data, u64 attr_id)
{
	struct ufs_hba *hba = data;

	if (!hba)
		return -EINVAL;

	hba->debugfs_files.dme_local_attr_id = (u32) attr_id;

	return 0;
}

static int ufsdbg_dme_local_read(void *data, u64 *attr_val)
{
	return ufsdbg_dme_read(data, attr_val, false);
}

DEFINE_SIMPLE_ATTRIBUTE(ufsdbg_dme_local_read_ops,
			ufsdbg_dme_local_read,
			ufsdbg_dme_local_set_attr_id, "%llu\n");

static int ufsdbg_dme_peer_read(void *data, u64 *attr_val)
{
	struct ufs_hba *hba = data;

	if (!hba)
		return -EINVAL;
	else
		return ufsdbg_dme_read(data, attr_val, true);
}

static int ufsdbg_dme_peer_set_attr_id(void *data, u64 attr_id)
{
	struct ufs_hba *hba = data;

	if (!hba)
		return -EINVAL;

	hba->debugfs_files.dme_peer_attr_id = (u32) attr_id;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ufsdbg_dme_peer_read_ops,
			ufsdbg_dme_peer_read,
			ufsdbg_dme_peer_set_attr_id, "%llu\n");

static ssize_t ufsdbg_req_stats_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	int val;
	int ret;
	unsigned long flags;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret) {
		dev_err(hba->dev, "%s: Invalid argument\n", __func__);
		return ret;
	}

	spin_lock_irqsave(hba->host->host_lock, flags);
	ufshcd_init_req_stats(hba);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
	return cnt;
}

static int ufsdbg_req_stats_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	int i;
	unsigned long flags;

	if (!hba)
		goto exit;

	/* Header */
	seq_printf(file, "\t%-10s %-10s %-10s %-10s %-10s %-10s",
		   "All", "Write", "Read", "Read(urg)", "Write(urg)", "Flush");

	spin_lock_irqsave(hba->host->host_lock, flags);

	seq_printf(file, "\n%s:\t", "Min");
	for (i = 0; i < TS_NUM_STATS; i++)
		seq_printf(file, "%-10llu ", hba->ufs_stats.req_stats[i].min);
	seq_printf(file, "\n%s:\t", "Max");
	for (i = 0; i < TS_NUM_STATS; i++)
		seq_printf(file, "%-10llu ", hba->ufs_stats.req_stats[i].max);
	seq_printf(file, "\n%s:\t", "Avg.");
	for (i = 0; i < TS_NUM_STATS; i++){
		if (0 == hba->ufs_stats.req_stats[i].count)
			continue;
		seq_printf(file, "%-10llu ",
			   div64_u64(hba->ufs_stats.req_stats[i].sum,
				     hba->ufs_stats.req_stats[i].count));
		}
	seq_puts(file, "\n");
	spin_unlock_irqrestore(hba->host->host_lock, flags);

exit:
	return 0;
}

static int ufsdbg_req_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_req_stats_show, inode->i_private);
}

static const struct file_operations ufsdbg_req_stats_desc = {
	.open = ufsdbg_req_stats_open,
	.read = seq_read,
	.write = ufsdbg_req_stats_write,
};

#ifdef CONFIG_HISI_DEBUG_FS
static int ufsdbg_idle_intr_verify_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	unsigned long flags;

	if (!hba)
		goto exit;

	spin_lock_irqsave(hba->host->host_lock, flags);
	seq_printf(file, "%d\n", hba->ufs_idle_intr_verify);
	spin_unlock_irqrestore(hba->host->host_lock, flags);

exit:
	return 0;
}

static int ufsdbg_idle_intr_verify_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_idle_intr_verify_show, inode->i_private);
}

static ssize_t ufsdbg_idle_intr_verify_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	int val;
	int ret;
	unsigned long flags;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret) {
		dev_err(hba->dev, "%s: Invalid argument\n", __func__);
		return ret;
	}

	spin_lock_irqsave(hba->host->host_lock, flags);
	hba->ufs_idle_intr_verify = val;
	spin_unlock_irqrestore(hba->host->host_lock, flags);
	return cnt;
}

static const struct file_operations ufsdbg_idle_intr_verify_desc = {
	.open = ufsdbg_idle_intr_verify_open,
	.read = seq_read,
	.write = ufsdbg_idle_intr_verify_write,
};

static int ufsdbg_idle_timeout_val_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;
	unsigned long flags;

	if (!hba)
		goto exit;

	spin_lock_irqsave(hba->host->host_lock, flags);
	seq_printf(file, "%dms\n", hba->idle_timeout_val / 1000);
	spin_unlock_irqrestore(hba->host->host_lock, flags);

exit:
	return 0;
}

static int ufsdbg_idle_timeout_val_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_idle_timeout_val_show, inode->i_private);
}

extern void ufs_idle_intr_toggle(struct ufs_hba *hba, int enable);
static ssize_t ufsdbg_idle_timeout_val_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	unsigned long flags;
	int val;
	int ret;
	struct blk_dev_lld *lld = &(hba->host->tag_set.lld_func);

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret) {
		dev_err(hba->dev, "%s: Invalid argument\n", __func__);
		return ret;
	}

	if (val <= 0) {
		dev_err(hba->dev, "%s: Invalid argument: %d\n", __func__, val);
		return cnt;
	}

	hba->idle_timeout_val = val * 1000;
	spin_lock_irqsave(hba->host->host_lock, flags);
	ufs_idle_intr_toggle(hba, atomic_read(&lld->hw_idle_en));
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	return cnt;
}

static const struct file_operations ufsdbg_idle_timeout_val_desc = {
	.open = ufsdbg_idle_timeout_val_open,
	.read = seq_read,
	.write = ufsdbg_idle_timeout_val_write,
};

static int ufsdbg_idle_intr_check_timer_threshold_show(struct seq_file *file, void *data)
{
	struct ufs_hba *hba = (struct ufs_hba *)file->private;

	if (!hba)
		goto exit;

	seq_printf(file, "%ds\n", hba->idle_intr_check_timer_threshold / 1000);

exit:
	return 0;
}

static int ufsdbg_idle_intr_check_timer_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufsdbg_idle_intr_check_timer_threshold_show, inode->i_private);
}

static ssize_t ufsdbg_idle_intr_check_timer_threshold_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct ufs_hba *hba = filp->f_mapping->host->i_private;
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret) {
		dev_err(hba->dev, "%s: Invalid argument\n", __func__);
		return ret;
	}

	if (val <= 0) {
		dev_err(hba->dev, "%s: Invalid argument: %d\n", __func__, val);
		return cnt;
	}

	hba->idle_intr_check_timer_threshold = val * 1000;

	return cnt;
}

static const struct file_operations ufsdbg_idle_intr_check_timer_threshold_desc = {
	.open = ufsdbg_idle_intr_check_timer_threshold_open,
	.read = seq_read,
	.write = ufsdbg_idle_intr_check_timer_threshold_write,
};

static int ufsdbg_add_timer_intr_debugfs(struct ufs_hba *hba)
{
	if (hba->ufs_idle_intr_en) {
		hba->debugfs_files.idle_intr_verify =
			debugfs_create_file("idle_intr_verify", S_IRUSR | S_IWUSR,
					hba->debugfs_files.debugfs_root, hba,
					&ufsdbg_idle_intr_verify_desc);
		if (!hba->debugfs_files.idle_intr_verify) {
			dev_err(hba->dev,
				"%s:  failed create idle_intr_verify debugfs entry\n",
				__func__);
			return -EFAULT;
		}

		hba->debugfs_files.idle_timeout_val =
			debugfs_create_file("idle_timeout_val", S_IRUSR | S_IWUSR,
					hba->debugfs_files.debugfs_root, hba,
					&ufsdbg_idle_timeout_val_desc);
		if (!hba->debugfs_files.idle_timeout_val) {
			dev_err(hba->dev,
				"%s:  failed create idle_timeout_val debugfs entry\n",
				__func__);
			return -EFAULT;
		}

		hba->debugfs_files.idle_intr_check_timer_threshold =
			debugfs_create_file("idle_intr_check_timer_threshold", S_IRUSR | S_IWUSR,
					hba->debugfs_files.debugfs_root, hba,
					&ufsdbg_idle_intr_check_timer_threshold_desc);
		if (!hba->debugfs_files.idle_intr_check_timer_threshold) {
			dev_err(hba->dev,
				"%s:  failed create idle_intr_check_timer_threshold debugfs entry\n",
				__func__);
			return -EFAULT;
		}
	}

	return 0;
}
#endif /* CONFIG_HISI_DEBUG_FS */

static int ufsdbg_add_stats_debugfs(struct ufs_hba *hba)
{
	hba->debugfs_files.tag_stats =
	    debugfs_create_file("tag_stats", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_tag_stats_fops);
	if (!hba->debugfs_files.tag_stats) {
		dev_err(hba->dev, "%s:  NULL tag stats file, exiting",
			__func__);
		return -EFAULT;
	}
	hba->debugfs_files.err_stats =
	    debugfs_create_file("err_stats", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_err_stats_fops);
	if (!hba->debugfs_files.err_stats) {
		dev_err(hba->dev, "%s:  NULL err stats file, exiting",
			__func__);
		return -EFAULT;
	}

	hba->debugfs_files.req_stats =
	    debugfs_create_file("req_stats", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_req_stats_desc);
	if (!hba->debugfs_files.req_stats) {
		dev_err(hba->dev,
			"%s:  failed create req_stats debugfs entry\n",
			__func__);
		return -EFAULT;
	}

	return 0;
}

struct ufs_hba *hba_addr;
EXPORT_SYMBOL(hba_addr);
void ufsdbg_add_debugfs(struct ufs_hba *hba)
{
	if (!hba) {
		pr_err("%s: NULL hba, exiting\n", __func__);
		return;
	}

	hba_addr = hba;
	hba->debugfs_files.debugfs_root = debugfs_create_dir("ufs", NULL);
	if (IS_ERR(hba->debugfs_files.debugfs_root))
		/* Don't complain -- debugfs just isn't enabled */
		goto err_no_root;
	if (!hba->debugfs_files.debugfs_root) {
		/*
		 * Complain -- debugfs is enabled, but it failed to
		 * create the directory
		 */
		dev_err(hba->dev,
			"%s: NULL debugfs root directory, exiting", __func__);
		goto err_no_root;
	}

	if (ufshcd_init_statistics(hba)) {
		dev_err(hba->dev, "%s: Error initializing statistics",
			__func__);
		goto err;
	}

	hba->debugfs_files.host_regs = debugfs_create_file("host_regs", S_IRUSR,
							   hba->debugfs_files.
							   debugfs_root, hba,
							   &ufsdbg_host_regs_fops);
	if (!hba->debugfs_files.host_regs) {
		dev_err(hba->dev, "%s:  NULL hcd regs file, exiting", __func__);
		goto err;
	}

	hba->debugfs_files.show_hba = debugfs_create_file("show_hba", S_IRUSR,
							  hba->debugfs_files.
							  debugfs_root, hba,
							  &ufsdbg_show_hba_fops);
	if (!hba->debugfs_files.show_hba) {
		dev_err(hba->dev, "%s:  NULL hba file, exiting", __func__);
		goto err;
	}

	hba->debugfs_files.dump_dev_desc =
	    debugfs_create_file("dump_device_desc", S_IRUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_dump_device_desc);
	if (!hba->debugfs_files.dump_dev_desc) {
		dev_err(hba->dev,
			"%s:  NULL dump_device_desc file, exiting", __func__);
		goto err;
	}

	hba->debugfs_files.power_mode =
	    debugfs_create_file("power_mode", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_power_mode_desc);
	if (!hba->debugfs_files.power_mode) {
		dev_err(hba->dev,
			"%s:  NULL power_mode_desc file, exiting", __func__);
		goto err;
	}

	hba->debugfs_files.dme_local_read =
	    debugfs_create_file("dme_local_read", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_dme_local_read_ops);
	if (!hba->debugfs_files.dme_local_read) {
		dev_err(hba->dev,
			"%s:  failed create dme_local_read debugfs entry\n",
			__func__);
		goto err;
	}

	hba->debugfs_files.dme_peer_read =
	    debugfs_create_file("dme_peer_read", S_IRUSR | S_IWUSR,
				hba->debugfs_files.debugfs_root, hba,
				&ufsdbg_dme_peer_read_ops);
	if (!hba->debugfs_files.dme_peer_read) {
		dev_err(hba->dev,
			"%s:  failed create dme_peer_read debugfs entry\n",
			__func__);
		goto err;
	}

	if (ufsdbg_add_stats_debugfs(hba))
		goto err;

#ifdef CONFIG_HISI_DEBUG_FS
	if (ufsdbg_add_timer_intr_debugfs(hba))
		goto err;
#endif /* CONFIG_HISI_DEBUG_FS */

	if (hba->vops && hba->vops->add_debugfs)
		hba->vops->add_debugfs(hba, hba->debugfs_files.debugfs_root);

	return;

err:
	debugfs_remove_recursive(hba->debugfs_files.debugfs_root);
	hba->debugfs_files.debugfs_root = NULL;
err_no_root:
	dev_err(hba->dev, "%s: failed to initialize debugfs\n", __func__);
}

void ufsdbg_remove_debugfs(struct ufs_hba *hba)
{
	if (hba->vops && hba->vops->remove_debugfs)
		hba->vops->remove_debugfs(hba);
	debugfs_remove_recursive(hba->debugfs_files.debugfs_root);
	kfree(hba->ufs_stats.tag_stats);

}
