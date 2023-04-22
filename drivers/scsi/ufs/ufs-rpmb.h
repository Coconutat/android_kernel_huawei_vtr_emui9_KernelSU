#include "ufshcd.h"
#ifndef _UFS_RPMB_H_
#define _UFS_RPMB_H_
#define MAX_RPMB_REGION_NUM 4
#define MAX_RPMB_REGION_UNIT_SIZE (128 * 1024)
#define RPMB_ACCESS_PM_RUNTIME_DELAY_TIME 5000
void ufs_get_rpmb_info(struct ufs_hba *hba);
void ufs_rpmb_pm_runtime_delay_enable(struct work_struct *work);
void ufs_rpmb_pm_runtime_delay_process(struct ufs_hba *hba);
#endif