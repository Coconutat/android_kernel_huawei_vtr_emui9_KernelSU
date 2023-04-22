/*
 * Encryption policy functions for per-f2fs sdp file encryption support.
 *
 * Copyright (C) 2017, Huawei, Ltd..
 *
 * Written by Li minfeng, 2017.
 *
 */
#include <keys/user-type.h>
#include <linux/printk.h>
#include <linux/mount.h>
#include <linux/f2fs_fs.h>
#include "f2fs.h"
#include "xattr.h"
#include "f2fs_sdp.h"

#if DEFINE_F2FS_FS_SDP_ENCRYPTION
static int f2fs_inode_is_config_encryption(struct inode *inode)
{
	if (!inode->i_sb->s_cop->get_context)
		return 0;
	return (inode->i_sb->s_cop->get_context(inode, NULL, 0L, NULL) > 0);
}

static int f2fs_inode_set_config_sdp_ece_encryption_flags(struct inode *inode,
							  void *fs_data)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;
	u32 flags;

	res = sb->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res)
		return res;

	flags |= F2FS_XATTR_SDP_ECE_CONFIG_FLAG;

	res = sb->s_sdp_cop->set_sdp_encrypt_flags(inode, fs_data, &flags);
	pr_sdp_info("f2fs_sdp %s:inode(%lu) config sdp ece ctx flag %u res %d\n",
		__func__, inode->i_ino, flags, res);
	return res;
}

static int f2fs_inode_set_config_sdp_sece_encryption_flags(struct inode *inode,
							   void *fs_data)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;
	u32 flags;

	res = sb->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res)
		return res;

	flags |= F2FS_XATTR_SDP_SECE_CONFIG_FLAG;

	res = sb->s_sdp_cop->set_sdp_encrypt_flags(inode, fs_data, &flags);
	pr_sdp_info("f2fs_sdp %s:inode(%lu) config sdp sece ctx flag %u res %d\n",
		__func__, inode->i_ino, flags, res);
	return res;
}

static int f2fs_create_sdp_encryption_context_from_policy(struct inode *inode,
					const struct fscrypt_sdp_policy *policy)
{
	struct f2fs_sdp_fscrypt_context sdp_ctx = { 0 };
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	u8 master_key_descriptor_tmp[FS_KEY_DESCRIPTOR_SIZE];
	int res = 0;

	if (!policy)
		return -EINVAL;

	pr_sdp_info("f2fs_sdp %s:inode (%lu) sdpclass %u\n",
		__func__, inode->i_ino, policy->sdpclass);

	if ((policy->sdpclass != FSCRYPT_SDP_ECE_CLASS)
	&& (policy->sdpclass != FSCRYPT_SDP_SECE_CLASS))
		return -EINVAL;
	if (!fscrypt_valid_enc_modes(policy->contents_encryption_mode,
				     policy->filenames_encryption_mode))
		return -EINVAL;

	if (policy->flags & ~FS_POLICY_FLAGS_VALID)
		return -EINVAL;

	/* sece keyring lock screen will only delete privkey,
	 * ece will delete keyring,but keyring check is right,
	 * for sece can create file in lock screen
	 */
	memcpy(master_key_descriptor_tmp, policy->master_key_descriptor,
		FS_KEY_DESCRIPTOR_SIZE);
	res = f2fs_inode_check_policy_keyring(master_key_descriptor_tmp,
			policy->sdpclass, 0);
	if (res)
		return res;

	if (!sb->s_sdp_cop->set_sdp_context
		|| !sb->s_sdp_cop->get_sdp_encrypt_flags
		|| !sb->s_sdp_cop->set_sdp_encrypt_flags)
		return -EOPNOTSUPP;

	sdp_ctx.format = FS_ENCRYPTION_CONTEXT_FORMAT_V2;
	memcpy(sdp_ctx.master_key_descriptor, policy->master_key_descriptor,
					FS_KEY_DESCRIPTOR_SIZE);

	sdp_ctx.contents_encryption_mode = policy->contents_encryption_mode;
	sdp_ctx.filenames_encryption_mode = policy->filenames_encryption_mode;
	sdp_ctx.flags = policy->flags;
	sdp_ctx.sdpclass = policy->sdpclass;
	sdp_ctx.version = policy->version;

	res = sb->s_sdp_cop->set_sdp_context(inode, &sdp_ctx,
		sizeof(sdp_ctx), NULL);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp ctx from policy failed "
		"res %d.\n", __func__, inode->i_ino, res);
		return res;
	}

	if (policy->sdpclass == FSCRYPT_SDP_ECE_CLASS)
		res = f2fs_inode_set_config_sdp_ece_encryption_flags(inode,
				NULL);
	else
		res = f2fs_inode_set_config_sdp_sece_encryption_flags(inode,
				NULL);
	if (res)
		pr_err("f2fs_sdp %s: inode(%lu) set sdp config flags failed, "
		"res %d.\n", __func__, inode->i_ino, res);

	if (S_ISREG(inode->i_mode)
	    && !res
	    && (inode->i_crypt_info))
		res = f2fs_change_to_sdp_crypto(inode, NULL);

	return res;
}

static int f2fs_is_sdp_context_consistent_with_policy(struct inode *inode,
					const struct fscrypt_sdp_policy *policy)
{
	struct f2fs_sdp_fscrypt_context ctx;
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;

	if (!sb->s_sdp_cop->get_sdp_context)
		return 0;

	res = sb->s_sdp_cop->get_sdp_context(inode, &ctx, sizeof(ctx), NULL);
	if (res != sizeof(ctx))
		return 0;

	return (memcmp(ctx.master_key_descriptor, policy->master_key_descriptor,
			FS_KEY_DESCRIPTOR_SIZE) == 0 &&
			(ctx.sdpclass == policy->sdpclass) &&
			(ctx.flags == policy->flags) &&
			(ctx.contents_encryption_mode ==
			 policy->contents_encryption_mode) &&
			(ctx.filenames_encryption_mode ==
			 policy->filenames_encryption_mode));
}

static int f2fs_fscrypt_ioctl_set_sdp_policy(struct file *filp,
					     const void __user *arg)
{
	struct fscrypt_sdp_policy policy;
	struct inode *inode = file_inode(filp);
	int ret;
	u32 flag;

	if (!inode_owner_or_capable(inode))
		return -EACCES;

	if (copy_from_user(&policy, arg, sizeof(policy)))
		return -EFAULT;

	if (policy.version != 0)
		return -EINVAL;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);
	down_write(&F2FS_I(inode)->i_sdp_sem);

	ret = f2fs_inode_get_sdp_encrypt_flags(inode, NULL, &flag);
	if (ret)
		goto err;
	if (!f2fs_inode_is_config_encryption(inode)) {
		ret = -EINVAL;
	} else if (!F2FS_INODE_IS_CONFIG_SDP_ENCRYPTION(flag)) {
		ret = f2fs_create_sdp_encryption_context_from_policy(inode,
								&policy);
	} else if (!f2fs_is_sdp_context_consistent_with_policy(inode,
								&policy)) {
		pr_warn("f2fs_sdp %s: Policy inconsistent with sdp context\n",
		__func__);
		ret = -EINVAL;
	}

err:
	up_write(&F2FS_I(inode)->i_sdp_sem);
	inode_unlock(inode);

	mnt_drop_write_file(filp);
	return ret;
}

static int f2fs_fscrypt_ioctl_get_sdp_policy(struct file *filp,
					     void __user *arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sdp_fscrypt_context ctx;
	struct fscrypt_sdp_policy policy;
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;

	if (!test_hw_opt(sb, SDP_ENCRYPT))
		return -EOPNOTSUPP;

	if (!sb->s_sdp_cop->get_sdp_context)
		return -ENODATA;

	res = sb->s_sdp_cop->get_sdp_context(inode, &ctx, sizeof(ctx), NULL);

	pr_sdp_info("f2fs_sdp %s:inode (%lu) res %d ctxsize %lu\n",
		__func__, inode->i_ino, res, sizeof(ctx));

	if (res != sizeof(ctx))
		return -ENODATA;

	if (ctx.format != FS_ENCRYPTION_CONTEXT_FORMAT_V2)
		return -EINVAL;

	policy.version = ctx.version;
	policy.sdpclass = ctx.sdpclass;
	policy.contents_encryption_mode = ctx.contents_encryption_mode;
	policy.filenames_encryption_mode = ctx.filenames_encryption_mode;
	policy.flags = ctx.flags;
	memcpy(policy.master_key_descriptor, ctx.master_key_descriptor,
				FS_KEY_DESCRIPTOR_SIZE);

	if (copy_to_user(arg, &policy, sizeof(policy)))
		return -EFAULT;
	return 0;
}

static int f2fs_fscrypt_ioctl_get_policy_type(struct file *filp,
					      void __user *arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	struct fscrypt_info *ci = inode->i_crypt_info;
	struct fscrypt_policy_type policy = { 0 };
	int res, flags;

	if (!test_hw_opt(sb, SDP_ENCRYPT))
		return -EOPNOTSUPP;

	if (!ci)
		goto out;

	policy.contents_encryption_mode = ci->ci_data_mode;
	policy.filenames_encryption_mode = ci->ci_filename_mode;
	policy.version = 0;
	policy.encryption_type = FSCRYPT_CE_CLASS;
	memcpy(policy.master_key_descriptor, ci->ci_master_key,
		FS_KEY_DESCRIPTOR_SIZE);

	if (!sb->s_sdp_cop->get_sdp_encrypt_flags)
		goto out;

	res = sb->s_sdp_cop->get_sdp_encrypt_flags(inode, NULL, &flags);
	pr_sdp_info("f2fs_sdp %s:inode(%lu) key %p flags %d res %d\n",
		__func__, inode->i_ino, ci->ci_key, flags, res);
	if (res)
		goto out;

	if (flags & F2FS_XATTR_SDP_ECE_ENABLE_FLAG)
		policy.encryption_type = FSCRYPT_SDP_ECE_CLASS;
	else if (flags & F2FS_XATTR_SDP_SECE_ENABLE_FLAG)
		policy.encryption_type = FSCRYPT_SDP_SECE_CLASS;
	else
		goto out;
out:
	if (copy_to_user(arg, &policy, sizeof(policy)))
		return -EFAULT;
	return 0;
}

int f2fs_inode_get_sdp_encrypt_flags(struct inode *inode, void *fs_data,
				     u32 *flag)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);

	if (!sb->s_sdp_cop->get_sdp_encrypt_flags)
		return -EOPNOTSUPP;
	return sb->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data, flag);
}

int f2fs_inode_set_enabled_sdp_ece_encryption_flags(struct inode *inode,
						    void *fs_data)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;
	u32 flags;

	res = sb->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res)
		return res;

	flags |= F2FS_XATTR_SDP_ECE_ENABLE_FLAG;

	res = sb->s_sdp_cop->set_sdp_encrypt_flags(inode, fs_data, &flags);
	pr_sdp_info("f2fs_sdp %s:inode(%lu) enable sdp ece ctx flag %u res %d\n",
		__func__, inode->i_ino, flags, res);
	return res;
}

int f2fs_inode_set_enabled_sdp_sece_encryption_flags(struct inode *inode,
						     void *fs_data)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res;
	u32 flags;

	res = sb->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data, &flags);
	if (res)
		return res;

	flags |= F2FS_XATTR_SDP_SECE_ENABLE_FLAG;

	res = sb->s_sdp_cop->set_sdp_encrypt_flags(inode, fs_data, &flags);
	pr_sdp_info("f2fs_sdp %s:inode(%lu) enable sdp sece ctx flag %u res %d\n",
		__func__, inode->i_ino, flags, res);
	return res;
}

int f2fs_inode_check_policy_keyring(u8 *descriptor, u8 sdpclass,
				    u8 enforce_check)
{
	struct key *keyring_key;
	const struct user_key_payload *ukp;
	struct fscrypt_sdp_key *master_sdp_key;
	int res = 0;

	keyring_key = fscrypt_request_key(descriptor,
		FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key)) {
		pr_err("f2fs_sdp %s: request_key failed!\n", __func__);
		pr_sdp_info("%02x%02x%02x%02x%02x%02x%02x%02x\n",
		descriptor[0], descriptor[1], descriptor[2], descriptor[3],
		descriptor[4], descriptor[5], descriptor[6], descriptor[7]);
		return PTR_ERR(keyring_key);
	}

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: key type must be logon\n", __func__);
		res = -ENOKEY;
		goto out;
	}

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		/* key was revoked before we acquired its semaphore */
		pr_err("f2fs_sdp %s: key was revoked\n", __func__);
		res = -EKEYREVOKED;
		up_read(&keyring_key->sem);
		goto out;
	}
	if (ukp->datalen != sizeof(struct fscrypt_sdp_key)) {
		pr_err("f2fs_sdp %s: sdp full key size incorrect: %d\n",
		__func__, ukp->datalen);
		res = -EINVAL;
		up_read(&keyring_key->sem);
		goto out;
	}

	master_sdp_key = (struct fscrypt_sdp_key *)ukp->data;
	if (master_sdp_key->sdpclass == FSCRYPT_SDP_ECE_CLASS) {
		if (sdpclass != FSCRYPT_SDP_ECE_CLASS)
			res = -EINVAL;
	} else if (master_sdp_key->sdpclass == FSCRYPT_SDP_SECE_CLASS) {
		if (sdpclass != FSCRYPT_SDP_SECE_CLASS)
			res = -EINVAL;
		else if (enforce_check) {
			if (master_sdp_key->size == 0)
				res = -EINVAL;
			else {
				u8 sdp_pri_key[FS_MAX_KEY_SIZE] = { 0 };

				if (memcmp(master_sdp_key->raw, sdp_pri_key,
				master_sdp_key->size) == 0)
					res = -EINVAL;
			}

			pr_sdp_info("f2fs_sdp %s: raw size %u res %d\n",
			__func__, master_sdp_key->size, res);
		}
	} else {
		res = -EINVAL;
	}

	up_read(&keyring_key->sem);
out:
	key_put(keyring_key);
	return res;
}
int f2fs_ioc_set_sdp_encryption_policy(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (!test_hw_opt(sbi, SDP_ENCRYPT))
		return -EOPNOTSUPP;

	f2fs_update_time(F2FS_I_SB(inode), REQ_TIME);

	return f2fs_fscrypt_ioctl_set_sdp_policy(filp,
		(const void __user *)arg);
}

int f2fs_ioc_get_sdp_encryption_policy(struct file *filp, unsigned long arg)
{
	return f2fs_fscrypt_ioctl_get_sdp_policy(filp, (void __user *)arg);
}

int f2fs_ioc_get_encryption_policy_type(struct file *filp, unsigned long arg)
{
	return f2fs_fscrypt_ioctl_get_policy_type(filp, (void __user *)arg);
}

int f2fs_is_permitted_context(struct inode *parent, struct inode *child)
{
	if (S_ISREG(child->i_mode)
	    && child->i_crypt_info
	    && F2FS_INODE_IS_ENABLED_SDP_ENCRYPTION(child->i_crypt_info->ci_sdp_flag))
		return 1;

	return 0;
}

int f2fs_sdp_crypt_inherit(struct inode *parent, struct inode *child,
			   void *dpage, void *fs_data)
{
	struct f2fs_sb_info *sb = F2FS_I_SB(parent);
	struct f2fs_sdp_fscrypt_context sdp_ctx;
	int res;

	/* if no sdp, no need to inherit the sdp context for the contextis
	 * is hidder feature inherit is invalid
	 */
	if (!test_hw_opt(sb, SDP_ENCRYPT))
		return 0;

	if (!sb->s_sdp_cop->get_sdp_context)
		return 0;

	res = sb->s_sdp_cop->get_sdp_context(parent, &sdp_ctx,
		sizeof(struct f2fs_sdp_fscrypt_context), dpage);
	if (res != sizeof(struct f2fs_sdp_fscrypt_context))
		return 0;
	if (S_ISREG(child->i_mode)) {
		res = f2fs_inode_check_policy_keyring(
			sdp_ctx.master_key_descriptor, sdp_ctx.sdpclass, 0);
		pr_sdp_info("f2fs_sdp %s:inode(%lu) check keyring(%lu) res %d.\n",
			__func__, child->i_ino, parent->i_ino, res);
		if (res)
			return res;
	}

	down_write(&F2FS_I(child)->i_sdp_sem);
	res = sb->s_sdp_cop->set_sdp_context(child, &sdp_ctx,
		sizeof(struct f2fs_sdp_fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp ctx from parent(%lu) "
		"failed, res %d\n", __func__, child->i_ino, parent->i_ino, res);
		up_write(&F2FS_I(child)->i_sdp_sem);
		return -EINVAL;
	}

	if (sdp_ctx.sdpclass == FSCRYPT_SDP_ECE_CLASS)
		res = f2fs_inode_set_config_sdp_ece_encryption_flags(child,
			fs_data);
	else if (sdp_ctx.sdpclass == FSCRYPT_SDP_SECE_CLASS)
		res = f2fs_inode_set_config_sdp_sece_encryption_flags(child,
			fs_data);
	else
		res = -EOPNOTSUPP;
	up_write(&F2FS_I(child)->i_sdp_sem);

	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp config flags failed, "
		"res %d.\n", __func__, child->i_ino, res);
		return -EINVAL;
	}

	return 0;
}
#endif

