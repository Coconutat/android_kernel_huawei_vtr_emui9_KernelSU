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
 * UFS fault inject - add fault inject interface to the ufshcd.
 * This infrastructure can be used for debugging the driver from userspace.
 *
 */

#ifndef _UFS_FAULT_INJECT_H
#define _UFS_FAULT_INJECT_H

#include "ufshcd.h"

#define CMD_HANG_ERROR              (0x0)
#define BUFFER_SIZE                 (128)

enum ufsdbg_err_inject_scenario {
	ERR_INJECT_INTR,
	ERR_INJECT_UIC_INTR_CAUSE,
	ERR_INJECT_UIC_CMD_ERR,
	ERR_INJECT_PWR_MODE_CHANGE_ERR,
	ERR_INJECT_TRANSFER_OCS,
	ERR_INJECT_TRANSFER_RSP,
	ERR_INJECT_SCSI_STATUS,
	ERR_INJECT_TM_OCS,
	ERR_INJECT_TM_RSP,
	ERR_INJECT_DEDBUG_CTRL_INTR,
	ERR_INJECT_MAX_ERR_SCENARIOS,
};

/* TM Overall command status values */
enum {
	TM_OCS_SUCCESS                   = 0x0,
	TM_OCS_INVALID_TM_FUNCTION_ATTR  = 0x01,
	TM_OCS_MISMATCH_TM_REQUEST_SIZE  = 0x02,
	TM_OCS_MISMATCH_TM_RESPONSE_SIZE = 0x03,
	TM_OCS_PEER_COMM_FAILURE         = 0x04,
	TM_OCS_ABORTED                   = 0x05,
	TM_OCS_FATAL_ERROR               = 0x06,
	TM_OCS_INVALID_OCS_VALUE         = 0x0f,
};

/* UTP Response Values*/
enum {
	TARGET_SUCCESS = 0x0,
	TARGET_FAIL    = 0x1,
};

#ifdef CONFIG_SCSI_UFS_FAULT_INJECT

void ufs_fault_inject_fs_setup(void);
void ufs_fault_inject_fs_remove(void);
void ufsdbg_error_inject_dispatcher(struct ufs_hba *hba,
			enum ufsdbg_err_inject_scenario err_scenario,
			int success_value, int *ret_value);
#else
static inline void ufsdbg_error_inject_dispatcher(struct ufs_hba *hba,
			enum ufsdbg_err_inject_scenario err_scenario,
			int success_value, int *ret_value)
{
}
static inline void ufs_fault_inject_fs_setup(void)
{
}
static inline void ufs_fault_inject_fs_remove(void)
{
}
#endif /* End of CONFIG_SCSI_UFS_FAULT_INJECT */

#endif /* End of Header */
