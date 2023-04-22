/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include "ufs-rpmb.h"
#ifdef CONFIG_HISI_BOOTDEVICE
#include <linux/bootdevice.h>
#endif

#ifdef CONFIG_PM
/**
 * ufs_rpmb_pm_runtime_delay_enable - when rpmb_pm_work in workqueue is scheduled, after 1 second pm_runtime dalay time set 5ms
* @work: pointer to work structure
 *
 */
void ufs_rpmb_pm_runtime_delay_enable(struct work_struct *work){
	struct ufs_hba *hba;
	/*lint -e826*/
	hba = container_of(work, struct ufs_hba, rpmb_pm_work);
	/*lint -e826*/
	msleep(3000);

	if (ufshcd_is_auto_hibern8_allowed(hba))
		ufshcd_set_auto_hibern8_delay(hba, UFS_AHIT_AUTOH8_TIMER);
	else
		pm_runtime_set_autosuspend_delay(hba->dev, 5);
}

/**
 * ufs_rpmb_pm_runtime_delay_process - rpmb request issue, and pm_runtime dalay time set 5000ms, work in queue is scheduled
 * @hba: pointer to adapter instance
 *
 */
void ufs_rpmb_pm_runtime_delay_process(struct ufs_hba *hba)
{
	if (ufshcd_is_auto_hibern8_allowed(hba)) {
		if (hba->ahit == UFS_AHIT_AUTOH8_TIMER) {
			ufshcd_set_auto_hibern8_delay(
				hba, UFS_AHIT_AUTOH8_RPMB_TIMER);
			schedule_work(&hba->rpmb_pm_work);
		}
	} else {
		if (hba->dev->power.autosuspend_delay == 5) {
			pm_runtime_set_autosuspend_delay(
				hba->dev, RPMB_ACCESS_PM_RUNTIME_DELAY_TIME);
			schedule_work(&hba->rpmb_pm_work);
		}
	}
}
#else
void ufs_rpmb_pm_runtime_delay_enable(struct work_struct *work)
{
}

void ufs_rpmb_pm_runtime_delay_process(struct ufs_hba *hba)
{
}
#endif

/**
 * ufs_get_rpmb_info - get rpmb info from device and set the rpmb config
 * @hba: pointer to adapter instance
 *
 */
void ufs_get_rpmb_info(struct ufs_hba *hba)
{
	int err;
	u8 desc_buf[QUERY_DESC_RPMB_UNIT_MAZ_SIZE] = {0};
	struct rpmb_config_info rpmb_config = {0};
	int i;
	/*get rpmb unit size*/
	rpmb_config.rpmb_unit_size = MAX_RPMB_REGION_UNIT_SIZE;

	err = ufshcd_read_unit_desc_param(hba, UFS_UPIU_RPMB_WLUN, UNIT_DESC_PARAM_LEN, desc_buf,
		QUERY_DESC_RPMB_UNIT_MAZ_SIZE);
	if (err) {
		dev_err(hba->dev, "%s: Failed getting rpmb info\n", __func__);
		goto out;
	}
	/* get rpmb total blks*/
	rpmb_config.rpmb_total_blks = (u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 0] << 56 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 1] << 48 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 2] << 40 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 3] << 32 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 4] << 24 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 5] << 16 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 6] << 8 |
	(u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 7] << 0;
	/*general rpmb_logical_block_size 8, means the size is 256Byte*/
	rpmb_config.rpmb_blk_size = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_SIZE];
	rpmb_config.rpmb_region_enable = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION_ENABLE];
	/*if rpmb is not support multi key, we do something.default is zero, so we set 0x1, means support 1 region*/
	if(rpmb_config.rpmb_region_enable == 0x0)
		rpmb_config.rpmb_region_enable = 0x1;
	rpmb_config.rpmb_region_size[0] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION0_SIZE];
	rpmb_config.rpmb_region_size[1] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION1_SIZE];
	rpmb_config.rpmb_region_size[2] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION2_SIZE];
	rpmb_config.rpmb_region_size[3] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION3_SIZE];
	/*if rpmb is not support multi key, we do something.default is zero, so we set region 1 size is total rpmb size*/
	if((rpmb_config.rpmb_region_size[0] | rpmb_config.rpmb_region_size[1] | rpmb_config.rpmb_region_size[2] | rpmb_config.rpmb_region_size[3]) == 0x0){
		rpmb_config.rpmb_region_size[0] = (u8)((rpmb_config.rpmb_total_blks << rpmb_config.rpmb_blk_size) / rpmb_config.rpmb_unit_size);
	}

	set_rpmb_total_blks(rpmb_config.rpmb_total_blks );
	set_rpmb_blk_size(rpmb_config.rpmb_blk_size);
	set_rpmb_unit_size(rpmb_config.rpmb_unit_size);
	set_rpmb_region_enable(rpmb_config.rpmb_region_enable);
	set_rpmb_read_align(rpmb_config.rpmb_read_align);
	set_rpmb_write_align(rpmb_config.rpmb_write_align);
	for(i = 0;i < MAX_RPMB_REGION_NUM;i++){
		set_rpmb_region_size(i, rpmb_config.rpmb_region_size[i]);
	}
	set_rpmb_config_ready_status();

out:
	return;
}
