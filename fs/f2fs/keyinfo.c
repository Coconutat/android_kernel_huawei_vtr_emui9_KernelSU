/*
 * Encryption policy functions for per-f2fs sdp file encryption support.
 *
 * Copyright (C) 2017, Huawei, Ltd..
 *
 * Written by Li minfeng, 2017.
 *
 */
#include <keys/user-type.h>
#include <linux/scatterlist.h>
#include <uapi/linux/keyctl.h>
#include <crypto/skcipher.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <linux/f2fs_fs.h>
#include "f2fs.h"
#include "xattr.h"
#include "f2fs_sdp.h"

#if DEFINE_F2FS_FS_SDP_ENCRYPTION
static struct key *f2fs_fscrypt_request_key(u8 *descriptor, u8 *prefix,
	int prefix_size)
{
	return fscrypt_request_key(descriptor, prefix, prefix_size);
}

static void f2fs_put_crypt_info(struct fscrypt_info *ci)
{
	void *prev;
	void *key;

	if (!ci)
		return;

	/*lint -save -e529 -e438*/
	key = ACCESS_ONCE(ci->ci_key);
	/*lint -restore*/
	/*lint -save -e1072 -e747 -e50*/
	prev = cmpxchg(&ci->ci_key, key, NULL);
	/*lint -restore*/
	if (prev == key && key) {
		memzero_explicit(key, (size_t)FS_MAX_KEY_SIZE);
		kfree(key);
		ci->ci_key_len = 0;
		ci->ci_key_index = -1;
	}

	crypto_free_skcipher(ci->ci_ctfm);
	if (ci->ci_gtfm)
		crypto_free_aead(ci->ci_gtfm);
	kmem_cache_free(fscrypt_info_cachep, ci);
}

static int f2fs_get_keyinfo_from_keyring(struct key *keyring_key,
	u8 keyring_type, u8 *raw, u32 *size)
{
	const struct user_key_payload *ukp;
	struct fscrypt_key *master_key;
	struct fscrypt_sdp_key *master_sdp_key;
	int res;

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		/* key was revoked before we acquired its semaphore */
		printk_once(KERN_WARNING "f2fs_sdp %s: key was revoked\n",
			__func__);
		res = -EKEYREVOKED;
		return res;
	}
	pr_sdp_info("f2fs_sdp %s : key serial () type %u datalen %u\n",
		__func__, keyring_type, ukp->datalen);
	if (keyring_type == FSCRYPT_CE_CLASS) {
		if (ukp->datalen != sizeof(struct fscrypt_key)) {
			printk_once(KERN_WARNING
			"f2fs_sdp %s: fscrypt key full size incorrect: %d\n",
			__func__, ukp->datalen);
			res = -EINVAL;
			return  res;
		}
		master_key = (struct fscrypt_key *)ukp->data;

		if (master_key->size != FS_AES_256_GCM_KEY_SIZE) {
			printk_once(KERN_WARNING
			"f2fs_sdp %s: fscrypt key size incorrect: %d\n",
			__func__, master_key->size);
			res = -ENOKEY;
			return res;
		}

		*size = master_key->size;
		memcpy(raw, master_key->raw, master_key->size);
	} else {
		if (ukp->datalen != sizeof(struct fscrypt_sdp_key)) {
			printk_once(KERN_WARNING
			"f2fs_sdp %s: sdp full key size incorrect: %d\n",
			__func__, ukp->datalen);
			res = -EINVAL;
			return res;
		}
		master_sdp_key = (struct fscrypt_sdp_key *)ukp->data;

		if (master_sdp_key->sdpclass == FSCRYPT_SDP_ECE_CLASS) {

			if (master_sdp_key->size != FS_AES_256_GCM_KEY_SIZE) {
				printk_once(KERN_WARNING
				"f2fs_sdp %s: sdp key size incorrect: %d\n",
				__func__, master_sdp_key->size);
				res = -ENOKEY;
				return res;
			}

			*size = master_sdp_key->size;
			memcpy(raw, master_sdp_key->raw, master_sdp_key->size);
		}
	}
	return 0;
}

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
static int f2fs_derive_get_keyindex(u8 *descriptor, u8 keyring_type)
{
	struct key *keyring_key;
	const struct user_key_payload *ukp;
	struct fscrypt_key *master_key;
	struct fscrypt_sdp_key *master_sdp_key;
	int res = 0;

	keyring_key = f2fs_fscrypt_request_key(descriptor,
		FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key)) {
		pr_err("f2fs_sdp %s: request_key failed!\n", __func__);
		return PTR_ERR(keyring_key);
	}

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: key type must be logon\n", __func__);
		res = -ENOKEY;
		return res;
	}

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		up_read(&keyring_key->sem);
		/* key was revoked before we acquired its semaphore */
		printk_once(KERN_WARNING "f2fs_sdp %s: key was revoked\n",
			__func__);
		res = -EKEYREVOKED;
		goto out;
	}

	if (keyring_type == FSCRYPT_CE_CLASS) {
		if (ukp->datalen != sizeof(struct fscrypt_key)) {
			up_read(&keyring_key->sem);
			printk_once(KERN_WARNING
			"f2fs_sdp %s: fscrypt key full size incorrect: %d\n",
			__func__, ukp->datalen);
			res = -EINVAL;
			goto out;
		}
		master_key = (struct fscrypt_key *)ukp->data;

		if (master_key->size != FS_AES_256_GCM_KEY_SIZE) {
			up_read(&keyring_key->sem);
			printk_once(KERN_WARNING
			"f2fs_sdp %s: fscrypt key size incorrect: %d\n",
			__func__, master_key->size);
			res = -ENOKEY;
			goto out;
		}

		res = (int) (*(master_key->raw + FS_KEY_INDEX_OFFSET) & 0xff);
	} else {
		if (ukp->datalen != sizeof(struct fscrypt_sdp_key)) {
			up_read(&keyring_key->sem);
			printk_once(KERN_WARNING
			"f2fs_sdp %s: sdp full key size incorrect: %d\n",
			__func__, ukp->datalen);
			res = -EINVAL;
			goto out;
		}

		master_sdp_key = (struct fscrypt_sdp_key *)ukp->data;

		if (master_sdp_key->sdpclass == FSCRYPT_SDP_ECE_CLASS) {
			if (master_sdp_key->size != FS_AES_256_GCM_KEY_SIZE) {
				up_read(&keyring_key->sem);
				printk_once(KERN_WARNING
				"f2fs_sdp %s: sdp key size incorrect: %d\n",
				__func__, master_sdp_key->size);
				res = -ENOKEY;
				goto out;
			}

			res = (int) (*(master_sdp_key->raw + FS_KEY_INDEX_OFFSET) & 0xff);
		}
	}

	up_read(&keyring_key->sem);
out:
	key_put(keyring_key);
	return res;
}
#endif

static int f2fs_derive_special_key(u8 *descriptor, u8 keyring_type,
	u8 *nonce, u8 *dst_key, u8 *iv, int enc)
{
	struct key *keyring_key;
	struct crypto_aead *tfm = NULL;
	u8 raw_key[FS_MAX_KEY_SIZE];
	int key_len;
	int res;

	keyring_key = f2fs_fscrypt_request_key(descriptor,
		FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key)) {
		pr_err("f2fs_sdp %s: request_key failed!\n", __func__);
		return PTR_ERR(keyring_key);
	}

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: key type must be logon\n", __func__);
		res = -ENOKEY;
		return res;
	}

	res = f2fs_get_keyinfo_from_keyring(keyring_key, keyring_type,
	raw_key, &key_len);
	if (res) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: get keyinfo from keyring fail! res %d\n",
			__func__, res);
		goto out;
	}

	tfm = (struct crypto_aead *)crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		up_read(&keyring_key->sem);
		res = (int)PTR_ERR(tfm);
		tfm = NULL;
		pr_err("f2fs_sdp %s: tfm allocation failed!\n", __func__);
		goto out;
	}

	res = fscrypt_set_gcm_key(tfm, raw_key);
	up_read(&keyring_key->sem);
	if (res) {
		pr_err("f2fs_sdp %s: set gcm key failed res %d\n",
			__func__, res);
		goto out;
	}

	res = fscrypt_derive_gcm_key(tfm, nonce, dst_key, iv, enc);
	if (res)
		pr_err("f2fs_sdp %s: derive_gcm_key failed res %d\n",
			__func__, res);
out:
	crypto_free_aead(tfm);
	key_put(keyring_key);
	return res;
}

static int f2fs_determine_cipher_type(struct fscrypt_info *ci,
	struct inode *inode, const char **cipher_str_ret, int *keysize_ret)
{
	if (S_ISREG(inode->i_mode)) {
		if (ci->ci_data_mode == FS_ENCRYPTION_MODE_AES_256_XTS) {
			*cipher_str_ret = "xts(aes)";
			*keysize_ret = FS_AES_256_XTS_KEY_SIZE;
			return 0;
		}
		pr_warn_once("f2fs_sdp %s: unsupported content crypt mod %d node %lu\n",
			__func__, ci->ci_data_mode, inode->i_ino);
		return -ENOKEY;
	}

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode)) {
		if (ci->ci_filename_mode == FS_ENCRYPTION_MODE_AES_256_CTS) {
			*cipher_str_ret = "cts(cbc(aes))";
			*keysize_ret = FS_AES_256_CTS_KEY_SIZE;
			return 0;
		}
		pr_warn_once("f2fs_sdp %s: unsupported fsname crypt mod %d node %lu\n",
			__func__, ci->ci_filename_mode, inode->i_ino);
		return -ENOKEY;
	}

	pr_warn_once("f2fs_sdp %s: unsupported file type %d for inode %lu\n",
		__func__, (inode->i_mode & S_IFMT), inode->i_ino);
	return -ENOKEY;
}

static int f2fs_get_crypt_info_file_key(struct inode *inode,
	struct fscrypt_info *crypt_info, u8 *raw_key)
{
	const char *cipher_str;
	struct crypto_skcipher *ctfm = NULL;
	int res = 0;
	int keysize;

	res = f2fs_determine_cipher_type(crypt_info, inode,
		&cipher_str, &keysize);
	if (res)
		goto out;

	ctfm = crypto_alloc_skcipher(cipher_str, 0, 0);
	if (!ctfm || IS_ERR(ctfm)) {
		res = ctfm ? PTR_ERR(ctfm) : -ENOMEM;
		pr_err("f2fs_sdp %s: error %d (inode %u) allocating crypto tfm\n",
		__func__, res, (unsigned) inode->i_ino);
		goto out;
	}

	crypto_skcipher_clear_flags(ctfm, ~0);
	crypto_skcipher_set_flags(ctfm, CRYPTO_TFM_REQ_WEAK_KEY);
	res = crypto_skcipher_setkey(ctfm, raw_key, keysize);
	if (res)
		goto out;

	kzfree(crypt_info->ci_key);
	crypt_info->ci_key = kzalloc((size_t)FS_MAX_KEY_SIZE, GFP_NOFS);
	if (!crypt_info->ci_key) {
		res = -ENOMEM;
		goto out;
	}

	if (keysize > FS_MAX_KEY_SIZE) {
		res = -EOVERFLOW;
		goto out;
	}

	crypt_info->ci_key_len = keysize;
	/*lint -save -e732 -e747*/
	memcpy(crypt_info->ci_key, raw_key, crypt_info->ci_key_len);
	/*lint -restore*/

	if (crypt_info->ci_ctfm)
		crypto_free_skcipher(crypt_info->ci_ctfm);
	crypt_info->ci_ctfm = ctfm;
	pr_sdp_info("f2fs_sdp %s : (inode %lu) key %p\n", __func__,
		inode->i_ino, crypt_info->ci_key);
	return 0;
out:
	if (ctfm)
		crypto_free_skcipher(ctfm);
	return res;
}

static int f2fs_get_sdp_ece_crypt_info_from_context(struct inode *inode,
	struct fscrypt_context *ctx, struct f2fs_sdp_fscrypt_context *sdp_ctx,
	struct fscrypt_info *crypt_info, int inherit_key, void *fs_data)
{
	int res;
	u32 flag = 0;
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
	u8 nonce[FS_KEY_DERIVATION_NONCE_SIZE];

	if (inherit_key) {
		res = f2fs_derive_special_key(ctx->master_key_descriptor,
			FSCRYPT_CE_CLASS, ctx->nonce, nonce, ctx->iv, 0);
		if (res) {
			pr_err("f2fs_sdp %s: error %d (inode %lu) get ce key "
			"failed\n", __func__, res, inode->i_ino);
			return res;
		}
		memcpy(iv, ctx->iv, FS_KEY_DERIVATION_IV_SIZE);
	} else {
		/* get key from the node for reboot */
		res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
		if (res)
			return res;
		if (F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag)) {
			res = f2fs_derive_special_key(
				sdp_ctx->master_key_descriptor,
				FSCRYPT_SDP_ECE_CLASS,
				sdp_ctx->nonce, nonce, sdp_ctx->iv, 0);
			if (res) {
				pr_err("f2fs_sdp %s: error %d (inode %lu) get "
				"ece key fail!\n", __func__, res, inode->i_ino);
				return res;
			}
		} else {
			/* get nonce and iv */
			get_random_bytes(nonce, FS_KEY_DERIVATION_NONCE_SIZE);
			get_random_bytes(iv, FS_KEY_DERIVATION_IV_SIZE);
		}
	}

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
	crypt_info->ci_key_index = f2fs_derive_get_keyindex(
						ctx->master_key_descriptor,
						FSCRYPT_CE_CLASS);
	if (crypt_info->ci_key_index < 0 || crypt_info->ci_key_index > 31) {
		pr_err("ece_key %s: %d\n", __func__, crypt_info->ci_key_index);
		BUG();
	}
#endif

	res = f2fs_get_crypt_info_file_key(inode, crypt_info, nonce);
	if (res) {
		pr_err("f2fs_sdp %s: error %d (inode %lu) get file key fail!\n",
		__func__, res, inode->i_ino);
		return res;
	}

	if (F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag))
		return 0;

	res = f2fs_derive_special_key(sdp_ctx->master_key_descriptor,
		FSCRYPT_SDP_ECE_CLASS, nonce, sdp_ctx->nonce, iv, 1);
	if (res) {
		pr_err("f2fs_sdp %s: error %d (inode %lu) encrypt ece key "
		"fail!\n", __func__, res, inode->i_ino);
		return res;
	}

	memcpy(sdp_ctx->iv, iv,  FS_KEY_DERIVATION_IV_SIZE);

	return 0;
}

static int f2fs_get_sdp_ece_crypt_info(struct inode *inode, void *fs_data)
{
	struct fscrypt_context ctx;
	struct f2fs_sdp_fscrypt_context sdp_ctx;
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	struct fscrypt_info *crypt_info;
	int inherit_key = 0;
	int res;
	u32 flag;

	res = sb->s_sdp_cop->get_sdp_context(inode, &sdp_ctx,
			sizeof(sdp_ctx), fs_data);
	if (res != sizeof(sdp_ctx))
		return -EINVAL;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx),
		fs_data);
	if (res != sizeof(ctx))
		return -EINVAL;

	crypt_info = kmem_cache_alloc(fscrypt_info_cachep, GFP_NOFS);
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_flags = sdp_ctx.flags;
	crypt_info->ci_data_mode = sdp_ctx.contents_encryption_mode;
	crypt_info->ci_filename_mode = sdp_ctx.filenames_encryption_mode;
	crypt_info->ci_ctfm = NULL;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_essiv_tfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	crypt_info->ci_key_index = -1;
	memcpy(crypt_info->ci_master_key, sdp_ctx.master_key_descriptor,
		sizeof(crypt_info->ci_master_key));

	res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res)
		goto out;
	if (!F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag)
	    && i_size_read(inode))
		inherit_key = 1;

	res = f2fs_get_sdp_ece_crypt_info_from_context(inode, &ctx,
		&sdp_ctx, crypt_info, inherit_key, fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: get cypt info failed\n", __func__);
		goto out;
	}
	pr_sdp_info("f2fs_sdp %s: get sdp cyptinfo %p key %p len %u\n",
	__func__, crypt_info, crypt_info->ci_key, crypt_info->ci_key_len);
	crypt_info->ci_sdp_flag = F2FS_XATTR_SDP_ECE_ENABLE_FLAG;
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;

	if (F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag))
		goto out;

	/* should set sdp context back for getting the nonce */
	res = sb->s_sdp_cop->update_sdp_context(inode, &sdp_ctx,
		sizeof(struct f2fs_sdp_fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp ctx failed res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

	res = f2fs_inode_set_enabled_sdp_ece_encryption_flags(inode,
			fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: set sdp crypt flag failed! need to "
		"check!\n", __func__);
		goto out;
	}

	/* clear the ce crypto info */
	memset(ctx.nonce, 0, FS_KEY_DERIVATION_CIPHER_SIZE);
	memset(ctx.iv, 0, FS_KEY_DERIVATION_IV_SIZE);

	res = sb->s_sdp_cop->update_context(inode, &ctx,
		sizeof(struct fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) update ce ctx failed res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

out:
	if (res == -ENOKEY)
		res = 0;
	f2fs_put_crypt_info(crypt_info);
	return res;
}

static int f2fs_derive_sdp_sece_key(u8 *descriptor, u8 *pubkey,
	u8 haspubkey, u8 *nonce, u8 *dst_key, u8 *iv, int enc)
{
	struct key *keyring_key;
	struct crypto_aead *tfm = NULL;
	const struct user_key_payload *ukp;
	struct fscrypt_sdp_key *master_sdp_key;
	u8 raw_key[FS_AES_256_GCM_KEY_SIZE];
	int res;

	keyring_key = f2fs_fscrypt_request_key(descriptor,
		FS_KEY_DESC_PREFIX, FS_KEY_DESC_PREFIX_SIZE);
	if (IS_ERR(keyring_key)) {
		pr_err("f2fs_sdp %s: request_key failed!\n", __func__);
		return PTR_ERR(keyring_key);
	}

	down_read(&keyring_key->sem);
	if (keyring_key->type != &key_type_logon) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: key type must be logon\n", __func__);
		res = -ENOKEY;
		return res;
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
		pr_err("f2fs_sdp %s: sdp key full size incorrect: %d\n",
		__func__, ukp->datalen);
		res = -EINVAL;
		up_read(&keyring_key->sem);
		goto out;
	}
	master_sdp_key = (struct fscrypt_sdp_key *)ukp->data;

	if (master_sdp_key->sdpclass != FSCRYPT_SDP_SECE_CLASS) {
		pr_err("f2fs_sdp %s: sdp key type incorrect: %d\n",
		__func__, master_sdp_key->sdpclass);
		res = -ENOKEY;
		up_read(&keyring_key->sem);
		goto out;
	}

	if (!haspubkey)
		res = get_file_pubkey_shared_secret(ECC_CURVE_NIST_P256,
			master_sdp_key->pubkey, FS_KEY_DERIVATION_PUBKEY_SIZE,
			pubkey, FS_KEY_DERIVATION_PUBKEY_SIZE,
			raw_key, FS_AES_256_GCM_KEY_SIZE);
	else
		res = get_shared_secret(ECC_CURVE_NIST_P256,
			master_sdp_key->raw, FS_AES_256_GCM_KEY_SIZE,
			pubkey, FS_KEY_DERIVATION_PUBKEY_SIZE,
			raw_key, FS_AES_256_GCM_KEY_SIZE);

	if (res) {
		up_read(&keyring_key->sem);
		pr_err("f2fs_sdp %s: compute crypt info failed! res %d\n",
			__func__, res);
		goto out;
	}

	tfm = (struct crypto_aead *)crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		up_read(&keyring_key->sem);
		res = (int)PTR_ERR(tfm);
		tfm = NULL;
		pr_err("f2fs_sdp %s: tfm allocation failed!\n", __func__);
		goto out;
	}

	up_read(&keyring_key->sem);
	res = fscrypt_set_gcm_key(tfm, raw_key);
	if (res) {
		pr_err("f2fs_sdp %s: set gcm key failed res %d\n",
			__func__, res);
		goto out;
	}

	res = fscrypt_derive_gcm_key(tfm, nonce, dst_key, iv, enc);
	if (res)
		pr_err("f2fs_sdp %s: derive_gcm_key failed res %d\n",
			__func__, res);
out:
	crypto_free_aead(tfm);
	key_put(keyring_key);
	return res;
}

static int f2fs_get_sdp_sece_crypt_info_from_context(struct inode *inode,
	struct fscrypt_context *ctx, struct f2fs_sdp_fscrypt_context *sdp_ctx,
	struct fscrypt_info *crypt_info, int inherit_key, void *fs_data)
{
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
	u8 nonce[FS_KEY_DERIVATION_NONCE_SIZE];
	int res, haspubkey;
	u32 flag = 0;

	if (inherit_key) {
		res = f2fs_derive_special_key(ctx->master_key_descriptor,
			FSCRYPT_CE_CLASS, ctx->nonce, nonce, ctx->iv, 0);
		if (res) {
			pr_err("f2fs_sdp %s: error %d (inode %lu) get ce key "
			"fail!\n", __func__, res, inode->i_ino);
			return res;
		}
		memcpy(iv, ctx->iv, FS_KEY_DERIVATION_IV_SIZE);
		/* inherit means no pubkey, if has pubkey means the
		 * file key is special, can't inherit
		 */
		haspubkey = 0;
	} else {
		/* get key from the node for reboot */
		res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
		if (res)
			return res;
		if (F2FS_INODE_IS_ENABLED_SDP_SECE_ENCRYPTION(flag)) {
			haspubkey = 1;
			res = f2fs_derive_sdp_sece_key(
				sdp_ctx->master_key_descriptor,
				sdp_ctx->file_pub_key,
				haspubkey, sdp_ctx->nonce,
				nonce, sdp_ctx->iv, 0);
			if (res) {
				pr_err("f2fs_sdp %s: error %d (inode %lu) get "
				"sece key fail\n", __func__, res, inode->i_ino);
				return res;
			}
		} else {
			/* get nonce and iv */
			get_random_bytes(nonce, FS_KEY_DERIVATION_NONCE_SIZE);
			get_random_bytes(iv, FS_KEY_DERIVATION_IV_SIZE);
			haspubkey = 0;
		}
	}

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
	crypt_info->ci_key_index = f2fs_derive_get_keyindex(
							ctx->master_key_descriptor,
							FSCRYPT_CE_CLASS);
	if (crypt_info->ci_key_index < 0 || crypt_info->ci_key_index > 31) {
		pr_err("sece_class %s: %d\n", __func__, crypt_info->ci_key_index);
		BUG();
	}
#endif

	res = f2fs_get_crypt_info_file_key(inode, crypt_info, nonce);
	if (res) {
		pr_err("f2fs_sdp %s: error %d (inode %lu) get file key fail!\n",
		__func__, res, inode->i_ino);
		return res;
	}

	if (F2FS_INODE_IS_ENABLED_SDP_SECE_ENCRYPTION(flag))
		return 0;

	res = f2fs_derive_sdp_sece_key(sdp_ctx->master_key_descriptor,
		sdp_ctx->file_pub_key, haspubkey, nonce, sdp_ctx->nonce, iv,
		1);
	if (res) {
		pr_err("f2fs_sdp %s: error %d (inode %lu) encrypt ece key "
		"fail!\n", __func__, res, inode->i_ino);
		return res;
	}

	memcpy(sdp_ctx->iv, iv,  FS_KEY_DERIVATION_IV_SIZE);

	return 0;
}

static int f2fs_get_sdp_sece_crypt_info(struct inode *inode, void *fs_data)
{
	struct fscrypt_context ctx;
	struct f2fs_sdp_fscrypt_context sdp_ctx;
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	struct fscrypt_info *crypt_info;
	int inherit_key = 0;
	int res;
	u32 flag;

	res = sb->s_sdp_cop->get_sdp_context(inode, &sdp_ctx,
			sizeof(sdp_ctx), fs_data);
	if (res != sizeof(sdp_ctx))
		return -EINVAL;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx),
		fs_data);
	if (res != sizeof(ctx))
		return -EINVAL;

	crypt_info = kmem_cache_alloc(fscrypt_info_cachep, GFP_NOFS);
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_flags = sdp_ctx.flags;
	crypt_info->ci_data_mode = sdp_ctx.contents_encryption_mode;
	crypt_info->ci_filename_mode = sdp_ctx.filenames_encryption_mode;
	crypt_info->ci_ctfm = NULL;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_essiv_tfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	crypt_info->ci_key_index = -1;
	memcpy(crypt_info->ci_master_key, sdp_ctx.master_key_descriptor,
		sizeof(crypt_info->ci_master_key));

	res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res)
		goto out;
	if (!F2FS_INODE_IS_ENABLED_SDP_SECE_ENCRYPTION(flag)
	    && i_size_read(inode))
		inherit_key = 1;

	res = f2fs_get_sdp_sece_crypt_info_from_context(inode, &ctx,
		&sdp_ctx, crypt_info, inherit_key, fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: get cypt info failed\n", __func__);
		goto out;
	}
	pr_sdp_info("f2fs_sdp %s:get sdp cyptinfo %p key %p len %u\n",
	__func__, crypt_info, crypt_info->ci_key, crypt_info->ci_key_len);
	crypt_info->ci_sdp_flag = F2FS_XATTR_SDP_SECE_ENABLE_FLAG;
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;

	if (F2FS_INODE_IS_ENABLED_SDP_SECE_ENCRYPTION(flag))
		goto out;

	/*should set sdp context back for getting the nonce*/
	res = sb->s_sdp_cop->update_sdp_context(inode, &sdp_ctx,
		sizeof(struct f2fs_sdp_fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp ctx failed res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

	res = f2fs_inode_set_enabled_sdp_sece_encryption_flags(inode,
			fs_data);
	if (res) {
		pr_err("f2fs_sdp %s:set sdp crypt flag failed! need to check!\n",
		__func__);
		goto out;
	}

	/*clear the ce crypto info*/
	memset(ctx.nonce, 0, FS_KEY_DERIVATION_CIPHER_SIZE);
	memset(ctx.iv, 0, FS_KEY_DERIVATION_IV_SIZE);

	res = sb->s_sdp_cop->update_context(inode, &ctx,
		sizeof(struct fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) update ce ctx failed res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

out:
	if (res == -ENOKEY)
		res = 0;
	f2fs_put_crypt_info(crypt_info);
	return res;
}

int f2fs_change_to_sdp_crypto(struct inode *inode, void *fs_data)
{
	struct fscrypt_info *ci_info = inode->i_crypt_info;
	struct fscrypt_context ctx;
	struct f2fs_sdp_fscrypt_context sdp_ctx;
	struct f2fs_sb_info *sb = F2FS_I_SB(inode);
	int res, inherit = 0;
	u32 flag;

	if (!ci_info)
		return -EINVAL;

	res = sb->s_sdp_cop->get_sdp_context(inode, &sdp_ctx,
			sizeof(sdp_ctx), fs_data);
	if (res != sizeof(sdp_ctx))
		return -EINVAL;

	res = inode->i_sb->s_cop->get_context(inode, &ctx,
			sizeof(ctx), fs_data);
	if (res != sizeof(ctx))
		return -EINVAL;

	/* file is not null, ece should inherit ce nonece iv,
	 * sece also support
	 */
	if (i_size_read(inode))
		inherit = 1;

	/* need to check the res, if dirty need to back, now can't back */
	pr_sdp_info("f2fs_sdp %s: sdp class %u inherit %u\n", __func__,
		sdp_ctx.sdpclass, inherit);

	if (sdp_ctx.sdpclass == FSCRYPT_SDP_ECE_CLASS)
		res = f2fs_get_sdp_ece_crypt_info_from_context(inode, &ctx,
				&sdp_ctx, ci_info, inherit, fs_data);
	else if (sdp_ctx.sdpclass == FSCRYPT_SDP_SECE_CLASS)
		res = f2fs_get_sdp_sece_crypt_info_from_context(inode, &ctx,
				&sdp_ctx, ci_info, inherit, fs_data);
	else
		res = -EINVAL;
	if (res) {
		pr_err("f2fs_sdp %s: get cypt info failed\n", __func__);
		return res;
	}

	memcpy(ci_info->ci_master_key, sdp_ctx.master_key_descriptor,
		FS_KEY_DESCRIPTOR_SIZE);
	ci_info->ci_flags = sdp_ctx.flags;
	res = sb->s_sdp_cop->update_sdp_context(inode, &sdp_ctx,
		sizeof(struct f2fs_sdp_fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) set sdp ctx failed, res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

	res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res)
		goto out;

	if (F2FS_INODE_IS_CONFIG_SDP_ECE_ENCRYPTION(flag)) {
		res = f2fs_inode_set_enabled_sdp_ece_encryption_flags(inode,
			fs_data);
		ci_info->ci_sdp_flag = F2FS_XATTR_SDP_ECE_ENABLE_FLAG;
	} else {
		res = f2fs_inode_set_enabled_sdp_sece_encryption_flags(inode,
			fs_data);
		ci_info->ci_sdp_flag = F2FS_XATTR_SDP_SECE_ENABLE_FLAG;
	}
	if (res) {
		pr_err("f2fs_sdp %s: set cypt flags failed!need to check!\n",
		__func__);
		goto out;
	}

	/* clear the ce crypto info */
	memset(ctx.nonce, 0, FS_KEY_DERIVATION_CIPHER_SIZE);
	memset(ctx.iv, 0, FS_KEY_DERIVATION_IV_SIZE);

	res = sb->s_sdp_cop->update_context(inode, &ctx,
		sizeof(struct fscrypt_context), fs_data);
	if (res) {
		pr_err("f2fs_sdp %s: inode(%lu) update ce ctx failed res %d.\n",
		__func__, inode->i_ino, res);
		goto out;
	}

	return 0;
out:
	fscrypt_put_encryption_info(inode, ci_info);
	return res;
}

static int f2fs_check_sdp_keyring_info(struct inode *inode, void *fs_data)
{
	struct fscrypt_info *ci_info = inode->i_crypt_info;
	int res = 0;
	u8 sdpclass, enforce = 0;
	u32 flag;

	if (!ci_info)
		return 0;

	res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res)
		return res;

	if (F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag))
		sdpclass = FSCRYPT_SDP_ECE_CLASS;
	else {
		sdpclass = FSCRYPT_SDP_SECE_CLASS;
		enforce = 1;
	}

	res = f2fs_inode_check_policy_keyring(ci_info->ci_master_key, sdpclass,
		enforce);

	if (res) {
		pr_err("f2fs_sdp %s: node(%lu) check policy failed, res %d\n",
		__func__, inode->i_ino, res);
		res = -ENOKEY;
	}

	return res;
}

static int f2fs_get_sdp_crypt_info(struct inode *inode, void *fs_data)
{
	int res = 0;
	struct fscrypt_info *ci_info = inode->i_crypt_info;
	u32 flag;

	res = f2fs_inode_get_sdp_encrypt_flags(inode, fs_data, &flag);
	if (res)
		return res;

	if (!F2FS_INODE_IS_CONFIG_SDP_ENCRYPTION(flag))
		return -ENODATA;

	/* means should change from ce to sdp crypto or return ok */
	if (ci_info && F2FS_INODE_IS_ENABLED_SDP_ENCRYPTION(flag))
		return f2fs_check_sdp_keyring_info(inode, fs_data);

	if (ci_info && F2FS_INODE_IS_CONFIG_SDP_ENCRYPTION(flag))
		return f2fs_change_to_sdp_crypto(inode, fs_data);

	if (F2FS_INODE_IS_CONFIG_SDP_ECE_ENCRYPTION(flag))
		return f2fs_get_sdp_ece_crypt_info(inode, fs_data);

	return f2fs_get_sdp_sece_crypt_info(inode, fs_data);
}

int f2fs_get_crypt_keyinfo(struct inode *inode, void *fs_data, int *flag)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	int res = 0;
	u32 sdpflag;
	/* used to check need to get ce encrypt info,
	 * if 0 need get ce crypt info
	 */
	*flag = 0;

	/* means only file use the sdp key, symlink dir use the origin key */
	if (S_ISREG(inode->i_mode)) {
		if (!sbi->s_sdp_cop->get_sdp_encrypt_flags)
			return -EOPNOTSUPP;
		down_write(&F2FS_I(inode)->i_sdp_sem);
		res = sbi->s_sdp_cop->get_sdp_encrypt_flags(inode, fs_data,
							    &sdpflag);
		if (res) {
			*flag = 1;
			goto unlock;
		}

		/* if no sdp but file encrypt by sdp, then should return no key,
		 * then open not permit
		 */
		if (!test_hw_opt(sbi, SDP_ENCRYPT)) {
			if (F2FS_INODE_IS_ENABLED_SDP_ENCRYPTION(sdpflag)) {
				*flag = 1;
				res = -ENOKEY;
			}
			goto unlock;
		}

		if (F2FS_INODE_IS_CONFIG_SDP_ENCRYPTION(sdpflag)) {
			*flag = 1;
			res = f2fs_get_sdp_crypt_info(inode, fs_data);

			if (res)
				pr_err("f2fs_sdp %s: inode(%lu) get keyinfo failed, "
				"res %d\n", __func__, inode->i_ino, res);
		}
unlock:
		up_write(&F2FS_I(inode)->i_sdp_sem);
	}

	return res;
}
#endif

