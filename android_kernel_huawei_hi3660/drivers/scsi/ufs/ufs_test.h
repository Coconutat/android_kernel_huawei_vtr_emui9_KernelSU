/*
 * drivers/scsi/ufs/ufs_test.h
 *
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UFS_TEST_H_
#define _UFS_TEST_H_

#include "ufshci.h"
#include "ufshcd.h"
#include "ufs.h"

extern struct ufs_hba *hba_addr;
extern int ufshcd_map_sg(struct ufshcd_lrb *lrbp);
extern int ufshcd_send_command(struct ufs_hba *hba, unsigned int task_tag);
extern int ufshcd_compose_upiu(struct ufs_hba *hba, struct ufshcd_lrb *lrbp);
extern bool ufshcd_get_dev_cmd_tag(struct ufs_hba *hba, int *tag_out);
#endif /* _UNIPRO_H_ */
