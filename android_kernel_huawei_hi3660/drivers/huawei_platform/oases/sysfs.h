#ifndef _OASES_SYSFS_H
#define _OASES_SYSFS_H

struct oases_patch_info;

enum {
	ATKSYS_UNINIT,
	ATKSYS_INIT,
};

enum {
	ATKSYS_SUCC,
	ATKSYS_ERR_KSET_ADD,
	ATKSYS_ERR_KOBJ_ADD,
	ATKSYS_ERR_KOBJECT_EVENT,
};

int oases_sysfs_add_patch(struct oases_patch_info* info);
void oases_sysfs_init_patch(struct oases_patch_info *info);
void oases_sysfs_del_patch(struct oases_patch_info *info);

int oases_sysfs_init(void);
void oases_sysfs_destroy(void);

#endif /* _OASES_SYSFS_H */
