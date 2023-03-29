/*
 * Encryption policy functions for per-f2fs sdp file encryption support.
 *
 * Copyright (C) 2017, Huawei, Ltd..
 *
 * Written by Li minfeng, 2017.
 *
 */
#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include "f2fs.h"
#include "xattr.h"
#include "f2fs_sdp.h"

#if DEFINE_F2FS_FS_SDP_ENCRYPTION
static int f2fs_get_sdp_context(struct inode *inode, void *ctx,
		size_t len, void *fs_data)
{
	return f2fs_getxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
				F2FS_XATTR_NAME_ENCRYPTION_CONTEXT,
				ctx, len, fs_data, 0);
}

static int f2fs_set_sdp_context(struct inode *inode, const void *ctx,
		size_t len, void *fs_data)
{
	pr_sdp_info("f2fs_sdp %s :inode(%lu) set sdp context\n",
		__func__, inode->i_ino);
	return f2fs_setxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
			F2FS_XATTR_NAME_ENCRYPTION_CONTEXT,
			ctx, len, fs_data, XATTR_CREATE);
}

static int f2fs_update_sdp_context(struct inode *inode, const void *ctx,
		size_t len, void *fs_data)
{
	pr_sdp_info("f2fs_sdp %s :inode(%lu) update sdp context\n",
		__func__, inode->i_ino);
	return f2fs_setxattr(inode, F2FS_XATTR_INDEX_ECE_ENCRYPTION,
			F2FS_XATTR_NAME_ENCRYPTION_CONTEXT,
			ctx, len, fs_data, XATTR_REPLACE);
}

static int f2fs_update_context(struct inode *inode, const void *ctx,
		size_t len, void *fs_data)
{
	pr_sdp_info("f2fs_sdp %s :inode(%lu) set ce key info to all 0\n",
		__func__, inode->i_ino);
	return f2fs_setxattr(inode, F2FS_XATTR_INDEX_ENCRYPTION,
			F2FS_XATTR_NAME_ENCRYPTION_CONTEXT,
			ctx, len, fs_data, XATTR_REPLACE);
}

static int f2fs_get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
		u32 *flags)
{
	struct f2fs_xattr_header *hdr;
	struct page *xpage;
	int err = -EINVAL;

	if (!fs_data)
		down_read(&F2FS_I(inode)->i_sem);

	*flags = 0;
	hdr = get_xattr_header(inode, (struct page *)fs_data, &xpage);
	if (IS_ERR_OR_NULL(hdr)) {
		err = (long)PTR_ERR(hdr);
		goto out_unlock;
	}

	*flags = hdr->h_xattr_flags;
	err = 0;
	f2fs_put_page(xpage, 1);
out_unlock:
	if (!fs_data)
		up_read(&F2FS_I(inode)->i_sem);
	return err;
}

static int f2fs_set_sdp_encrypt_flags(struct inode *inode, void *fs_data,
		u32 *flags)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_xattr_header *hdr;
	struct page *xpage = NULL;
	int err = 0;

	if (!fs_data) {
		f2fs_lock_op(sbi);
		down_write(&F2FS_I(inode)->i_sem);
	}

	hdr = get_xattr_header(inode, (struct page *)fs_data, &xpage);
	if (IS_ERR_OR_NULL(hdr)) {
		err = (long)PTR_ERR(hdr);
		goto out_unlock;
	}

	hdr->h_xattr_flags = *flags;
	if (fs_data)
		set_page_dirty(fs_data);
	else if (xpage)
		set_page_dirty(xpage);

	f2fs_put_page(xpage, 1);

	f2fs_mark_inode_dirty_sync(inode, true);
	if (S_ISDIR(inode->i_mode))
		set_sbi_flag(sbi, SBI_NEED_CP);

out_unlock:
	if (!fs_data) {
		up_write(&F2FS_I(inode)->i_sem);
		f2fs_unlock_op(sbi);
	}
	return err;
}

const struct f2fs_sdp_fscrypt_operations f2fs_sdp_cryptops = {
	.get_sdp_context = f2fs_get_sdp_context,
	.set_sdp_context = f2fs_set_sdp_context,
	.update_sdp_context = f2fs_update_sdp_context,
	.update_context = f2fs_update_context,
	.get_sdp_encrypt_flags = f2fs_get_sdp_encrypt_flags,
	.set_sdp_encrypt_flags = f2fs_set_sdp_encrypt_flags,
};
#endif
