/*
 * Sysfs for OASES framework
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/kobject.h>
#include "util.h"
#include "patch_info.h"
#include "patch_base.h"
#include "patch_api.h"
#include "patch_mgr.h"
#include "sysfs.h"
#ifdef CONFIG_HW_ROOT_SCAN
#include <chipset_common/security/root_scan.h>
#endif

/*
 * Oases Sysfs Interface
 *
 * /sys/kernel/oases
 * /sys/kernel/oases/state
 * /sys/kernel/oases/version
 *
 * /sys/kernel/oases/<patch>
 * /sys/kernel/oases/<patch>/enabled (0/1)
 * /sys/kernel/oases/<patch>/log
 * /sys/kernel/oases/<patch>/info
 *
 * patch: $VENDOR-$NAME-$ID
 */

#define PROC_OASES_STATE "state"

#define SYSFS_OASES "oases"
#define SYSFS_OASES_VERSION "version"

#define SYSFS_PATCH_ENABLED "enabled"
#define SYSFS_PATCH_LOG "log"

#define ATTACK_KOBJ_NAME "oases_attack"
#define ATTACK_KSET_NAME "oases_attack_kset"

extern struct list_head patchinfo_list;
extern struct mutex oases_mutex;

static struct kobject *sysfs_oases = NULL;
struct kobject *attack_kobj;
struct kset *attack_kset;
int attack_upload_init;

static ssize_t enabled_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct oases_patch_info *info;

	info = container_of(kobj, struct oases_patch_info, kobj);
	return snprintf(buf, 16, "%u\n", info->status);
}

static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	long enabled = 0;
	struct oases_patch_info *info;

	if (!mutex_trylock(&oases_mutex))
		return -EBUSY;

	ret = kstrtol(buf, 10, &enabled);
	if (ret < 0) {
		ret = -EINVAL;
		goto end;
	}

	if (enabled != STATUS_ENABLED && enabled != STATUS_DISABLED) {
		ret = -EINVAL;
		goto end;
	}

	info = container_of(kobj, struct oases_patch_info, kobj);
	if (info->status == enabled) {
		ret = -EINVAL;
		goto end;
	}

#ifdef CONFIG_HW_ROOT_SCAN
	root_scan_pause(D_RSOPID_KCODE, NULL);
#endif
	if (enabled == STATUS_DISABLED)
		ret = oases_insn_unpatch(info);
	else /* STATUS_ENABLED */
		ret = oases_insn_patch(info);
#ifdef CONFIG_HW_ROOT_SCAN
	root_scan_resume(D_RSOPID_KCODE, NULL);
#endif

	if (ret) {
		oases_error("%s %d -> %ld failed with %d\n",
			info->id, info->status, enabled, ret);
		goto end;
	}

	info->status = enabled;
	ret = count;

end:
	mutex_unlock(&oases_mutex);
	return ret;
}

static ssize_t log_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct oases_patch_info *info;
	struct oases_attack_log *cur;
	int i;
	ssize_t ret = 0;
	unsigned long flags;

	info = container_of(kobj, struct oases_patch_info, kobj);
	spin_lock_irqsave(&info->log_lock, flags);
	for (i = 0; i < info->log_index; i++) {
		cur = info->plog + i;
		if (!cur->count)
			continue;
		ret += scnprintf(buf + ret, PAGE_SIZE - ret - 1, "%ld %ld %ld %ld\n", cur->uid,
						cur->count, cur->start_time, cur->end_time);
	}
	spin_unlock_irqrestore(&info->log_lock, flags);
	return ret;
}

static ssize_t info_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct oases_patch_info *ctx;
	struct oases_patch_entry *patch, *p;
	int count = 0;

	ctx = container_of(kobj, struct oases_patch_info, kobj);
	list_for_each_entry_safe(patch, p, &ctx->patches, list) {
		count += kp_vtab(patch)->show_info(patch, kobj, attr, buf, count);
	}

	return count;
}

static ssize_t version_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 16, "%u\n", OASES_VERSION);
}

static ssize_t state_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct oases_patch_info *info;
	ssize_t ret = 0;

	mutex_lock(&oases_mutex);
	list_for_each_entry(info, &patchinfo_list, list) {
		ret += scnprintf(buf + ret, PAGE_SIZE - ret - 1, "%s %s\n",
				info->id, info->status ? "enabled" : "disabled");
	}
	mutex_unlock(&oases_mutex);
	return ret;
}

static struct kobj_attribute attribute_version =
	__ATTR(version, S_IRUGO, version_show, NULL);

static struct kobj_attribute attribute_state =
	__ATTR(state, S_IRUGO, state_show, NULL);

static struct attribute *attrs[] = {
	&attribute_version.attr,
	&attribute_state.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobj_attribute attribute_enabled =
	__ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO, enabled_show, enabled_store);

static struct kobj_attribute attribute_log =
	__ATTR(log, S_IRUGO, log_show, NULL);

static struct kobj_attribute attribute_info =
	__ATTR(info, S_IRUGO, info_show, NULL);

static struct attribute *attrs_patch[] = {
	&attribute_enabled.attr,
	&attribute_log.attr,
	&attribute_info.attr,
	NULL
};

static void kobj_release_oases_patch(struct kobject *kobj)
{
	struct oases_patch_info *ctx;

	ctx = container_of(kobj, struct oases_patch_info, kobj);
	oases_free_patch(ctx);
}

static struct kobj_type ktype_oases_patch = {
	.release = kobj_release_oases_patch,
	.sysfs_ops = &kobj_sysfs_ops,
	.default_attrs = attrs_patch
};

void oases_sysfs_init_patch(struct oases_patch_info *info)
{
	kobject_init(&info->kobj, &ktype_oases_patch);
}

int oases_sysfs_add_patch(struct oases_patch_info *info)
{
	return kobject_add(&info->kobj, sysfs_oases, "%s", info->id);
}

void oases_sysfs_del_patch(struct oases_patch_info *info)
{
	kobject_put(&info->kobj);
}

static void attack_uploader_deinit(void)
{
	if (attack_kobj) {
		kobject_put(attack_kobj);
		attack_kobj = NULL;
	}

	if (attack_kset) {
		kset_unregister(attack_kset);
		attack_kset = NULL;
	}

	attack_upload_init = ATKSYS_UNINIT;
}

static int attack_uploader_init(void)
{
	int error = ATKSYS_SUCC;
	oases_error("attack_uploader_init start\n");
	if (attack_upload_init == ATKSYS_INIT) {
		return ATKSYS_SUCC;
	}
	do {
		attack_kobj = kobject_create_and_add(ATTACK_KOBJ_NAME,
			kernel_kobj);
		if (!attack_kobj) {
			error = ATKSYS_ERR_KOBJ_ADD;
			break;
		}

		attack_kset =  kset_create_and_add(ATTACK_KSET_NAME, NULL,
				kernel_kobj);
		if (!attack_kset) {
			error = ATKSYS_ERR_KSET_ADD;
			break;
		}
		attack_kobj->kset = attack_kset;

		error = kobject_uevent(attack_kobj, KOBJ_ADD);
		if (error) {
			error = ATKSYS_ERR_KOBJECT_EVENT;
			break;
		}

		/* init ok */
		oases_error("attack_uploader_init success\n");
		attack_upload_init = ATKSYS_INIT;
		return ATKSYS_SUCC;
	}while (0);

	attack_uploader_deinit();
	oases_error("attack_uploader_init fail\n");
	return error;
}

int __init oases_sysfs_init(void)
{
	int ret;

	sysfs_oases = kobject_create_and_add(SYSFS_OASES, kernel_kobj);
	if (!sysfs_oases) {
		return -ENOMEM;
	}
	attack_uploader_init();

	ret = sysfs_create_group(sysfs_oases, &attr_group);
	if (ret)
		goto create_group_fail;

	return 0;

create_group_fail:
	kobject_put(sysfs_oases);
	return ret;
}

void oases_sysfs_destroy(void)
{
	sysfs_remove_group(sysfs_oases, &attr_group);
	kobject_put(sysfs_oases);
}
