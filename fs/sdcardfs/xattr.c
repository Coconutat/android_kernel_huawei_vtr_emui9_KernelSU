/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/xattr.c
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd
 *   Authors: Daeho Jeong, Woojoong Lee, Seunghwan Hyun,
 *               Sunghwan Yun, Sungjong Seo
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include <linux/xattr.h>
#include "sdcardfs.h"

int sdcardfs_setxattr(struct dentry *dentry, const char *name,
	const void *value, size_t size, int flags)
{
	int rc;
	struct path lower_path;
	struct dentry *lower_dentry;
	const struct cred *saved_cred = NULL;

	/* save current_cred and override it */
	saved_cred = override_fsids(SDCARDFS_SB(dentry->d_inode->i_sb),
				    SDCARDFS_I(dentry->d_inode)->data);
	if (!saved_cred) {
	    return -ENOMEM;
	}
	sdcardfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);

	rc = vfs_setxattr(NULL, lower_dentry, name, value, size, flags);

	dput(lower_dentry);
	sdcardfs_put_lower_path(dentry, &lower_path);
	revert_fsids(saved_cred);
	return rc;
}

ssize_t sdcardfs_getxattr(struct dentry *dentry,
	const char *name, void *value, size_t size)
{
	ssize_t rc;
	struct path lower_path;
	struct dentry *lower_dentry;
	const struct cred *saved_cred = NULL;

	/* save current_cred and override it */
	saved_cred = override_fsids(SDCARDFS_SB(dentry->d_inode->i_sb),
				    SDCARDFS_I(dentry->d_inode)->data);
	if (!saved_cred) {
	    return -ENOMEM;
	}
	sdcardfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);

	rc = vfs_getxattr(NULL, lower_dentry, name, value, size);

	dput(lower_dentry);
	sdcardfs_put_lower_path(dentry, &lower_path);
	revert_fsids(saved_cred);
	return rc;
}

ssize_t sdcardfs_listxattr(struct dentry *dentry, char *list, size_t size)
{
	ssize_t rc;
	struct path lower_path;
	struct dentry *lower_dentry;
	const struct cred *saved_cred = NULL;

	/* save current_cred and override it */
	saved_cred = override_fsids(SDCARDFS_SB(dentry->d_inode->i_sb),
				    SDCARDFS_I(dentry->d_inode)->data);
	if (!saved_cred) {
	    return -ENOMEM;
	}
	sdcardfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);

	rc = vfs_listxattr(lower_dentry, list, size);

	dput(lower_dentry);
	sdcardfs_put_lower_path(dentry, &lower_path);
	revert_fsids(saved_cred);
	return rc;
}

int sdcardfs_removexattr(struct dentry *dentry, const char *name)
{
	ssize_t rc;
	struct path lower_path;
	struct dentry *lower_dentry;
	const struct cred *saved_cred = NULL;

	/* save current_cred and override it */
	saved_cred = override_fsids(SDCARDFS_SB(dentry->d_inode->i_sb),
				    SDCARDFS_I(dentry->d_inode)->data);
	if (!saved_cred) {
	    return -ENOMEM;
	}
	sdcardfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);

	rc = vfs_removexattr(NULL, lower_dentry, name);

	dput(lower_dentry);
	sdcardfs_put_lower_path(dentry, &lower_path);
	revert_fsids(saved_cred);
	return rc;
}

static int sdcardfs_xattr_get(const struct xattr_handler *handler,
			 struct dentry *dentry, struct inode *inode,
			 const char *name, void *value, size_t size)
{
	return (int)sdcardfs_getxattr(dentry, name, value, size);
}

static int sdcardfs_xattr_set(const struct xattr_handler *handler,
			  struct dentry *dentry, struct inode *inode,
			  const char *name, const void *value, size_t size,
			  int flags)
{
	if (!value)
		return sdcardfs_removexattr(dentry, name);

	return sdcardfs_setxattr(dentry, name, value, size, flags);
}
static const struct xattr_handler sdcardfs_xattr_handler = {
	.prefix = "",
	.get    = sdcardfs_xattr_get,
	.set    = sdcardfs_xattr_set,
};

const struct xattr_handler *sdcardfs_xattr_handlers[] = {
	&sdcardfs_xattr_handler,
	NULL
};
