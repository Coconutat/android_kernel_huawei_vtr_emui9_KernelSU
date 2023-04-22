/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/sysfs.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"

#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
static ssize_t
sdcardfs_sysfs_sb_blocked_users_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf) {

	struct sdcardfs_sb_info *sbi = container_of(kobj,
		struct sdcardfs_sb_info, kobj);
	ssize_t len = 0;

	if (sbi->blocked_userid >= 0)
		len = snprintf(buf, PAGE_SIZE, "%d %d",
			sbi->blocked_userid, sbi->appid_excluded);
	buf[len++] = '\n';
	buf[len++] = '\0';
	return len;
}

static ssize_t
sdcardfs_sysfs_sb_blocked_users_store(struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf, size_t len) {

	struct sdcardfs_sb_info *sbi = container_of(kobj,
		struct sdcardfs_sb_info, kobj);

	int args = sscanf(buf, "%d%d", &sbi->blocked_userid,
		&sbi->appid_excluded);

	if (args <= 0)
		sbi->blocked_userid = -1;
	else if (args <= 1)
		sbi->appid_excluded = -1;

	/* print some debug messages for the privacyspace feature */
	if (sbi->blocked_userid < 0)
		pr_warn("all users have access to %s now", kobj->name);
	else {
		pr_warn("user/%d has been blocked from accessing %s",
			sbi->blocked_userid, kobj->name);

		if (sbi->appid_excluded >= 0)
			pr_warn("but appid/%d will be excluded",
				sbi->appid_excluded);
	}
	return len;
}
#endif

static ssize_t
sdcardfs_sysfs_sb_device_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf) {

	struct sdcardfs_sb_info *sbi = container_of(kobj,
		struct sdcardfs_sb_info, kobj);

	return snprintf(buf, PAGE_SIZE, "%s\n", sbi->obbpath_s);
}

static void __sdcardfs_sysfs_sb_release(struct kobject *kobj)
{
	struct sdcardfs_sb_info *sbi = container_of(kobj,
		struct sdcardfs_sb_info, kobj);

	kfree(sbi);
}

static ssize_t
sdcardfs_sysfs_attr_show(struct kobject *kobj,
	struct attribute *attr, char *buf)
{
	struct kobj_attribute *ka =
		container_of(attr, struct kobj_attribute, attr);

	return ka->show(kobj, ka, buf);
}

static ssize_t
sdcardfs_sysfs_attr_store(struct kobject *kobj,
	struct attribute *attr, const char *buf,
	size_t len)
{
	struct kobj_attribute *ka;

	char *s = skip_spaces(buf);
	len -= s - buf;
	buf = s;

	if (len == 0 || *buf == '\0')
		return -EINVAL;

	ka = container_of(attr, struct kobj_attribute, attr);
	return ka->store(kobj, ka, buf, len);
}

#define __SYSFS_ATTR_RW(name)	\
	(struct kobj_attribute)__ATTR(name, S_IWUSR | S_IRUGO, \
sdcardfs_sysfs_sb_##name##_show, sdcardfs_sysfs_sb_##name##_store)

#define __SYSFS_ATTR_RO(name)	\
	(struct kobj_attribute)__ATTR(name, S_IRUGO, \
sdcardfs_sysfs_sb_##name##_show, NULL)

static struct sysfs_ops sysfs_op = {
	.show = sdcardfs_sysfs_attr_show,
	.store = sdcardfs_sysfs_attr_store
};

static struct attribute *sb_attrs[] = {
	&__SYSFS_ATTR_RO(device).attr,
#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
	&__SYSFS_ATTR_RW(blocked_users).attr,
#endif
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct kobj_type sb_ktype = {
	.release = __sdcardfs_sysfs_sb_release,
	.sysfs_ops = &sysfs_op,
	.default_attrs = sb_attrs,
};

static struct kset *sdcardfs_kset;

int sdcardfs_sysfs_init(void)
{
	/* located under /sys/fs/ */
	sdcardfs_kset = kset_create_and_add(SDCARDFS_NAME, NULL, fs_kobj);
	return sdcardfs_kset == NULL ? -ENOMEM : 0;
}

void sdcardfs_sysfs_exit(void)
{
	BUG_ON(sdcardfs_kset == NULL);

	kset_unregister(sdcardfs_kset);
}

int sdcardfs_sysfs_register_sb(struct super_block *sb)
{
	int err;
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(sb);

	BUG_ON(sdcardfs_kset == NULL);

	sbi->kobj.kset = sdcardfs_kset;
	err = kobject_init_and_add(&sbi->kobj, &sb_ktype, NULL,
		"%u:%u", MAJOR(sb->s_dev), MINOR(sb->s_dev));
	if (err) {
		//errln("failed to kobject_init_and_add, err=%d", err);
		pr_err("failed to kobject_init_and_add, err=%d", err);
		return err;
	}

	/* send the uevent that the kobject is added to the sysfs */
	kobject_uevent(&sbi->kobj, KOBJ_ADD);
	return 0;
}

