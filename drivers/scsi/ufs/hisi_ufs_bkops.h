#ifndef HISI_UFS_BKOPS_H
#define HISI_UFS_BKOPS_H
#include "ufshcd.h"

#define HISI_UFS_MODEL_ANY "HISI_UFS_MODEL_ANY"
#define HISI_UFS_REV_ANY "HISI_UFS_REV_ANY"

struct hisi_ufs_bkops_id {
	uint16_t manufacturer_id;
	char *ufs_model;
	char *ufs_rev;
	struct list_head p;
};

typedef int (ufs_bkops_query_fn)(struct ufs_hba *hba, u32 *status);
typedef int (ufs_bkops_start_fn)(struct ufs_hba *hba);
typedef int (ufs_bkops_stop_fn)(struct ufs_hba *hba);
struct ufs_dev_bkops_ops {
	ufs_bkops_query_fn *ufs_bkops_query;
	ufs_bkops_start_fn *ufs_bkops_start;
	ufs_bkops_stop_fn *ufs_bkops_stop;
};

#endif

