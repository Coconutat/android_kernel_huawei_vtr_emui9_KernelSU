/* 
 * Copyright (C) 2011-2013 Samsung India Software Operations
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * See the COPYING file in the top-level directory or visit
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This program is provided "AS IS" and "WITH ALL FAULTS" and
 * without warranty of any kind. You are solely responsible for
 * determining the appropriateness of using and distributing
 * the program and assume all risks associated with your exercise
 * of rights with respect to the program, including but not limited
 * to infringement of third party rights, the risks and costs of
 * program errors, damage to or loss of data, programs or equipment,
 * and unavailability or interruption of operations. Under no
 * circumstances will the contributor of this Program be liable for
 * any damages of any kind arising from your use or distribution of
 * this program.
 *
 * The Linux Foundation chooses to take subject only to the GPLv2
 * license terms, and distributes only under these terms.
 */

#define pr_fmt(fmt) "ufshcd :" fmt

#include "ufshcd.h"
#include <linux/hisi/kirin_partition.h>

static struct ufs_hba *ufs_hba_tmp = NULL;

void ufs_get_boot_partition_type(struct ufs_hba *hba)
{
	u32 boot_partition_type;
	int ret;

	ufs_hba_tmp = hba;
	ret = ufshcd_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR,
			QUERY_ATTR_IDN_BOOT_LU_EN, 0, 0, &boot_partition_type);

	if (ret) {
		dev_err(hba->dev, "%s: Failed getting boot partition type\n", __func__);
		ufs_boot_partition_type = XLOADER_A;
		return;
	}

	ufs_boot_partition_type = boot_partition_type;

	dev_info(hba->dev, "%s: ufs boot partition type is %s\n",
		__func__, (ufs_boot_partition_type == 0x1) ? "XLOADER_A" : "xloader_B");

	return;
}

int ufs_set_boot_partition_type(int boot_partition_type)
{
	int ret;

	ret = ufshcd_query_attr(ufs_hba_tmp, UPIU_QUERY_OPCODE_WRITE_ATTR,
			QUERY_ATTR_IDN_BOOT_LU_EN, 0, 0, &boot_partition_type);

	if (ret)
		dev_err(ufs_hba_tmp->dev, "%s: Failed setting boot partition type\n", __func__);

	return ret;
}

