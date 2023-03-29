#ifdef CONFIG_HUAWEI_UFS_VENDOR_MODE
#ifndef LINUX_UFS_VENOR_MODE_H
#define LINUX_UFS_VENOR_MODE_H
#include "ufshcd.h"

#define UFS_IOCTL_VENDOR_PACKAGE_COUNT_MAX 10
#define UFS_IOCTL_VENDOR_CDB_LEN 16
#define UFS_VENDOR_DATA_SIZE_MAX 0x100
#define UFS_IOCTL_VENDOR_CMD 6

struct ufs_vendor_cmd {
	unsigned char vendor_cdb[16];
	int data_flag;
	char *buf;
};

struct ufs_ioctl_vendor_state_t {
	struct ufs_vendor_cmd *vendor_cmd;
	int cmd_count;
};

int ufs_ioctl_vendor_package(
    struct ufs_hba *hba,
    struct ufs_ioctl_vendor_state_t *ufs_vendor_ioctl_state,
    void __user *buffer);
int ufs_ioctl_vendor_package_tick(
    struct ufs_hba *hba, struct scsi_device *dev,
    struct ufs_ioctl_vendor_state_t *ufs_vendor_ioctl_state,
    void __user *buffer);
#endif
#endif
