/*
 * key management facility for FS encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * This contains encryption key functions.
 *
 * Written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar, 2015.
 */

#include <keys/user-type.h>
#include <linux/scatterlist.h>
#include <linux/ratelimit.h>
#include <crypto/aes.h>
#include <crypto/sha.h>
#include "fscrypt_private.h"

static struct crypto_shash *essiv_hash_tfm;

static void derive_crypt_complete(struct crypto_async_request *req, int rc)
{
	struct fscrypt_completion_result *ecr = req->data;

	if (rc == -EINPROGRESS)
		return;

	ecr->res = rc;
	complete(&ecr->completion);
}

/**
 * derive_key_aes() - Derive a key using AES-128-ECB
 * @deriving_key: Encryption key used for derivation.
 * @source_key:   Source key to which to apply derivation.
 * @derived_raw_key:  Derived raw key.
 *
 * Return: Zero on success; non-zero otherwise.
 */
static int derive_key_aes(u8 deriving_key[FS_AES_128_ECB_KEY_SIZE],
				const struct fscrypt_key *source_key,
				u8 derived_raw_key[FS_MAX_KEY_SIZE])
{
	int res = 0;
	struct skcipher_request *req = NULL;
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct scatterlist src_sg, dst_sg;
	struct crypto_skcipher *tfm = crypto_alloc_skcipher("ecb(aes)", 0, 0);

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		tfm = NULL;
		goto out;
	}
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);
	req = skcipher_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}
	skcipher_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			derive_crypt_complete, &ecr);
	res = crypto_skcipher_setkey(tfm, deriving_key,
					FS_AES_128_ECB_KEY_SIZE);
	if (res < 0)
		goto out;

	sg_init_one(&src_sg, source_key->raw, source_key->size);
	sg_init_one(&dst_sg, derived_raw_key, source_key->size);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg, source_key->size,
				   NULL);
	res = crypto_skcipher_encrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
	}
out:
	skcipher_request_free(req);
	crypto_free_skcipher(tfm);
	return res;
}

int fscrypt_set_gcm_key(struct crypto_aead *tfm,
			u8 deriving_key[FS_AES_256_GCM_KEY_SIZE])
{
	int res = 0;
	unsigned int iv_len;

	crypto_aead_set_flags(tfm, CRYPTO_TFM_REQ_WEAK_KEY);

	iv_len = crypto_aead_ivsize(tfm);
	if (iv_len > FS_KEY_DERIVATION_IV_SIZE) {
		res = -EINVAL;
		pr_err("fscrypt %s : IV length is incompatible\n", __func__);
		goto out;
	}

	res = crypto_aead_setauthsize(tfm, FS_KEY_DERIVATION_TAG_SIZE);
	if (res < 0) {
		pr_err("fscrypt %s : Failed to set authsize\n", __func__);
		goto out;
	}

	res = crypto_aead_setkey(tfm, deriving_key,
					FS_AES_256_GCM_KEY_SIZE);
	if (res < 0)
		pr_err("fscrypt %s : Failed to set deriving key\n", __func__);
out:
	return res;
}

int fscrypt_derive_gcm_key(struct crypto_aead *tfm,
				u8 source_key[FS_KEY_DERIVATION_CIPHER_SIZE],
				u8 derived_key[FS_KEY_DERIVATION_CIPHER_SIZE],
				u8 iv[FS_KEY_DERIVATION_IV_SIZE],
				int enc)
{
	int res = 0;
	struct aead_request *req = NULL;
	DECLARE_FS_COMPLETION_RESULT(ecr);
	struct scatterlist src_sg, dst_sg;
	unsigned int ilen;

	if (!tfm) {
		res = -EINVAL;
		goto out;
	}

	if (IS_ERR(tfm)) {
		res = PTR_ERR(tfm);
		goto out;
	}

	req = aead_request_alloc(tfm, GFP_NOFS);
	if (!req) {
		res = -ENOMEM;
		goto out;
	}

	aead_request_set_callback(req,
			CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
			derive_crypt_complete, &ecr);

	ilen = enc ? FS_KEY_DERIVATION_NONCE_SIZE :
			FS_KEY_DERIVATION_CIPHER_SIZE;

	sg_init_one(&src_sg, source_key, FS_KEY_DERIVATION_CIPHER_SIZE);
	sg_init_one(&dst_sg, derived_key, FS_KEY_DERIVATION_CIPHER_SIZE);

	aead_request_set_ad(req, 0);

	aead_request_set_crypt(req, &src_sg, &dst_sg, ilen, iv);

	res = enc ? crypto_aead_encrypt(req) : crypto_aead_decrypt(req);
	if (res == -EINPROGRESS || res == -EBUSY) {
		wait_for_completion(&ecr.completion);
		res = ecr.res;
	}

out:
	if (req)
		aead_request_free(req);
	return res;
}

struct key *fscrypt_request_key(u8 *descriptor, u8 *prefix,
				int prefix_size)
{
	u8 *full_key_descriptor;
	struct key *keyring_key;
	int full_key_len = prefix_size + (FS_KEY_DESCRIPTOR_SIZE * 2) + 1;

	full_key_descriptor = kmalloc(full_key_len, GFP_NOFS);
	if (!full_key_descriptor)
		return (struct key *)ERR_PTR(-ENOMEM);

	memcpy(full_key_descriptor, prefix, prefix_size);
	sprintf(full_key_descriptor + prefix_size,
			"%*phN", FS_KEY_DESCRIPTOR_SIZE,
			descriptor);
	full_key_descriptor[full_key_len - 1] = '\0';
	keyring_key = request_key(&key_type_logon, full_key_descriptor, NULL);
	kfree(full_key_descriptor);

	return keyring_key;
}

static int validate_user_key(struct fscrypt_info *crypt_info,
			struct fscrypt_context *ctx, u8 *raw_key,
			const char *prefix, int min_keysize)
{
	struct key *keyring_key;
	struct fscrypt_key *master_key;
	const struct user_key_payload *ukp;
	int prefix_size = strlen(prefix);
	int res;
	u8 plain_text[FS_KEY_DERIVATION_CIPHER_SIZE] = {0};
	struct crypto_aead *tfm = NULL;

	keyring_key = fscrypt_request_key(ctx->master_key_descriptor,
				prefix, prefix_size);
	if (IS_ERR(keyring_key))
		return PTR_ERR(keyring_key);
	down_read(&keyring_key->sem);

	ukp = user_key_payload_locked(keyring_key);
	if (!ukp) {
		/* key was revoked before we acquired its semaphore */
		res = -EKEYREVOKED;
		goto out;
	}
	if (ukp->datalen != sizeof(struct fscrypt_key)) {
		res = -EINVAL;
		goto out;
	}
	master_key = (struct fscrypt_key *)ukp->data;
	BUILD_BUG_ON(FS_AES_256_XTS_KEY_SIZE != FS_KEY_DERIVATION_NONCE_SIZE);

	crypt_info->ci_key_index = (int) *(master_key->raw + 63) & 0xff;

	//force the size equal to FS_AES_256_GCM_KEY_SIZE since user space might pass FS_AES_256_XTS_KEY_SIZE
	master_key->size = FS_AES_256_GCM_KEY_SIZE;
	if (master_key->size != FS_AES_256_GCM_KEY_SIZE ||
	    master_key->size > FS_MAX_KEY_SIZE
	    || master_key->size % AES_BLOCK_SIZE != 0) {
		printk_once(KERN_WARNING
				"%s: key size incorrect: %d\n",
				__func__, master_key->size);
		res = -ENOKEY;
		goto out;
	}

	tfm = (struct crypto_aead *)crypto_alloc_aead("gcm(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		up_read(&keyring_key->sem);
		res = (int)PTR_ERR(tfm);
		tfm = NULL;
		pr_err("fscrypt %s : tfm allocation failed!\n", __func__);
		goto out;
	}

	res = fscrypt_set_gcm_key(tfm, master_key->raw);
	if (res)
		goto out;
	res = fscrypt_derive_gcm_key(tfm, ctx->nonce, plain_text, ctx->iv, 0);
	if (res)
		goto out;

	memcpy(raw_key, plain_text, FS_KEY_DERIVATION_NONCE_SIZE);

	crypt_info->ci_gtfm = tfm;
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return 0;

out:
	if (tfm)
		crypto_free_aead(tfm);
	up_read(&keyring_key->sem);
	key_put(keyring_key);
	return res;
}

static const struct {
	const char *cipher_str;
	int keysize;
} available_modes[] = {
	[FS_ENCRYPTION_MODE_AES_256_XTS] = { "xts(aes)",
					     FS_AES_256_XTS_KEY_SIZE },
	[FS_ENCRYPTION_MODE_AES_256_CTS] = { "cts(cbc(aes))",
					     FS_AES_256_CTS_KEY_SIZE },
	[FS_ENCRYPTION_MODE_AES_128_CBC] = { "cbc(aes)",
					     FS_AES_128_CBC_KEY_SIZE },
	[FS_ENCRYPTION_MODE_AES_128_CTS] = { "cts(cbc(aes))",
					     FS_AES_128_CTS_KEY_SIZE },
};

static int determine_cipher_type(struct fscrypt_info *ci, struct inode *inode,
				 const char **cipher_str_ret, int *keysize_ret)
{
	u32 mode;

	if (!fscrypt_valid_enc_modes(ci->ci_data_mode, ci->ci_filename_mode)) {
		pr_warn_ratelimited("fscrypt: inode %lu uses unsupported encryption modes (contents mode %d, filenames mode %d)\n",
				    inode->i_ino,
				    ci->ci_data_mode, ci->ci_filename_mode);
		return -EINVAL;
	}

	if (S_ISREG(inode->i_mode)) {
		mode = ci->ci_data_mode;
	} else if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode)) {
		mode = ci->ci_filename_mode;
	} else {
		WARN_ONCE(1, "fscrypt: filesystem tried to load encryption info for inode %lu, which is not encryptable (file type %d)\n",
			  inode->i_ino, (inode->i_mode & S_IFMT));
		return -EINVAL;
	}

	*cipher_str_ret = available_modes[mode].cipher_str;
	*keysize_ret = available_modes[mode].keysize;
	return 0;
}

static void put_crypt_info(struct fscrypt_info *ci)
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
	crypto_free_cipher(ci->ci_essiv_tfm);
	kmem_cache_free(fscrypt_info_cachep, ci);
}

static int fscrypt_verify_ctx(struct fscrypt_context *ctx)
{
	if ((u32)(ctx->format) != FS_ENCRYPTION_CONTEXT_FORMAT_V2)
		return -EINVAL;

	if (!fscrypt_valid_enc_modes((u32)(ctx->contents_encryption_mode),
			(u32)(ctx->filenames_encryption_mode)))
		return -EINVAL;

	if ((u32)(ctx->flags) & ~FS_POLICY_FLAGS_VALID)
		return -EINVAL;

	return 0;
}

/*
 * When we cannot determine if original or backup ctx is valid,
 * trust original if @verify is 0, or backup if it is 1.
 */
int fscrypt_get_verify_context(struct inode *inode, void *ctx, size_t len)
{
	if (!inode->i_sb->s_cop->get_verify_context)
		return 0;

	return inode->i_sb->s_cop->get_verify_context(inode, ctx, len);
}

int fscrypt_set_verify_context(struct inode *inode, const void *ctx,
			size_t len, void *fs_data, int create_crc)
{
	if (!inode->i_sb->s_cop->set_verify_context)
		return 0;

	return inode->i_sb->s_cop->set_verify_context(inode,
				ctx, len, fs_data, create_crc);
}

static int derive_essiv_salt(const u8 *key, int keysize, u8 *salt)
{
	struct crypto_shash *tfm = READ_ONCE(essiv_hash_tfm);

	/* init hash transform on demand */
	if (unlikely(!tfm)) {
		struct crypto_shash *prev_tfm;

		tfm = crypto_alloc_shash("sha256", 0, 0);
		if (IS_ERR(tfm)) {
			pr_warn_ratelimited("fscrypt: error allocating SHA-256 transform: %ld\n",
					    PTR_ERR(tfm));
			return PTR_ERR(tfm);
		}
		prev_tfm = cmpxchg(&essiv_hash_tfm, NULL, tfm);
		if (prev_tfm) {
			crypto_free_shash(tfm);
			tfm = prev_tfm;
		}
	}

	{
		SHASH_DESC_ON_STACK(desc, tfm);
		desc->tfm = tfm;
		desc->flags = 0;

		return crypto_shash_digest(desc, key, keysize, salt);
	}
}

static int init_essiv_generator(struct fscrypt_info *ci, const u8 *raw_key,
				int keysize)
{
	int err;
	struct crypto_cipher *essiv_tfm;
	u8 salt[SHA256_DIGEST_SIZE];

	essiv_tfm = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(essiv_tfm))
		return PTR_ERR(essiv_tfm);

	ci->ci_essiv_tfm = essiv_tfm;

	err = derive_essiv_salt(raw_key, keysize, salt);
	if (err)
		goto out;

	/*
	 * Using SHA256 to derive the salt/key will result in AES-256 being
	 * used for IV generation. File contents encryption will still use the
	 * configured keysize (AES-128) nevertheless.
	 */
	err = crypto_cipher_setkey(essiv_tfm, salt, sizeof(salt));
	if (err)
		goto out;

out:
	memzero_explicit(salt, sizeof(salt));
	return err;
}

void __exit fscrypt_essiv_cleanup(void)
{
	crypto_free_shash(essiv_hash_tfm);
}

int fscrypt_get_encryption_info(struct inode *inode)
{
	struct fscrypt_info *crypt_info;
	struct fscrypt_context ctx;
	struct crypto_skcipher *ctfm;
	const char *cipher_str;
	int keysize;
	u8 *raw_key = NULL;
	int res;
	int has_crc = 0;
	int verify = 0;
	int flag = 0;

	if (inode->i_crypt_info && !inode->i_crypt_info->ci_sdp_flag)
		return 0;

	res = fscrypt_initialize(inode->i_sb->s_cop->flags);
	if (res)
		return res;

	if (inode->i_sb->s_cop && inode->i_sb->s_cop->get_keyinfo) {
		res = inode->i_sb->s_cop->get_keyinfo(inode, NULL, &flag);
		if (flag)
			return res;
	}

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx), &has_crc);
	if (res < 0) {
		if (!fscrypt_dummy_context_enabled(inode) ||
		    inode->i_sb->s_cop->is_encrypted(inode)) {
			verify = fscrypt_get_verify_context(inode, &ctx,
								    sizeof(ctx));
			if (verify < 0)
				inode->i_sb->s_cop->set_encrypted_corrupt(inode);
			return res;
		}
		/* Fake up a context for an unencrypted directory */
		memset(&ctx, 0, sizeof(ctx));
		ctx.format = FS_ENCRYPTION_CONTEXT_FORMAT_V2;
		ctx.contents_encryption_mode = FS_ENCRYPTION_MODE_AES_256_XTS;
		ctx.filenames_encryption_mode = FS_ENCRYPTION_MODE_AES_256_CTS;
		memset(ctx.master_key_descriptor, 0x42, FS_KEY_DESCRIPTOR_SIZE);
	} else if (res != sizeof(ctx)) {
		pr_err("%s: inode %lu incorrect ctx size [%u : %lu]\n",
			inode->i_sb->s_type->name, inode->i_ino, res, sizeof(ctx));
		inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		return -EINVAL;
	}


	if (fscrypt_verify_ctx(&ctx)) {
		pr_err("%s: inode %lu verify ctx failed\n",
			inode->i_sb->s_type->name, inode->i_ino);
		inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		return -EINVAL;
	}

	crypt_info = kmem_cache_alloc(fscrypt_info_cachep, GFP_NOFS);
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_flags = ctx.flags;
	crypt_info->ci_data_mode = ctx.contents_encryption_mode;
	crypt_info->ci_filename_mode = ctx.filenames_encryption_mode;
	crypt_info->ci_ctfm = NULL;
	crypt_info->ci_gtfm = NULL;
	crypt_info->ci_essiv_tfm = NULL;
	crypt_info->ci_key = NULL;
	crypt_info->ci_key_len = 0;
	crypt_info->ci_key_index = -1;
	crypt_info->ci_sdp_flag = 0;
	memcpy(crypt_info->ci_master_key, ctx.master_key_descriptor,
				sizeof(crypt_info->ci_master_key));

	res = determine_cipher_type(crypt_info, inode, &cipher_str, &keysize);
	if (res)
		goto out;

	/*
	 * This cannot be a stack buffer because it is passed to the scatterlist
	 * crypto API as part of key derivation.
	 */
	res = -ENOMEM;
	raw_key = kmalloc(FS_MAX_KEY_SIZE, GFP_NOFS);
	if (!raw_key)
		goto out;

	res = validate_user_key(crypt_info, &ctx, raw_key, FS_KEY_DESC_PREFIX,
				keysize);
	if (res && inode->i_sb->s_cop->key_prefix) {
		int res2 = validate_user_key(crypt_info, &ctx, raw_key,
					     inode->i_sb->s_cop->key_prefix,
					     keysize);
		if (res2) {
			verify = fscrypt_get_verify_context(inode, &ctx,
							    sizeof(ctx));
			if (verify < 0 || res2 == -EBADMSG || res == -EBADMSG)
				inode->i_sb->s_cop->set_encrypted_corrupt(inode);
			if (res2 == -ENOKEY)
				res = -ENOKEY;
			goto out;
		}
	} else if (res) {
		verify = fscrypt_get_verify_context(inode, &ctx, sizeof(ctx));
		if (verify < 0 || res == -EBADMSG)
			inode->i_sb->s_cop->set_encrypted_corrupt(inode);
		goto out;
	}
	ctfm = crypto_alloc_skcipher(cipher_str, 0, 0);
	if (IS_ERR(ctfm)) {
		res = PTR_ERR(ctfm);
		pr_debug("%s: error %d (inode %lu) allocating crypto tfm\n",
			 __func__, res, inode->i_ino);
		goto out;
	}
	crypt_info->ci_ctfm = ctfm;
	crypto_skcipher_set_flags(ctfm, CRYPTO_TFM_REQ_WEAK_KEY);
	/*
	 * if the provided key is longer than keysize, we use the first
	 * keysize bytes of the derived key only
	 */
	res = crypto_skcipher_setkey(ctfm, raw_key, keysize);
	if (res)
		goto out;

	if (S_ISREG(inode->i_mode) &&
			inode->i_sb->s_cop->is_inline_encrypted &&
			inode->i_sb->s_cop->is_inline_encrypted(inode)) {
		crypt_info->ci_key = kzalloc((size_t)FS_MAX_KEY_SIZE, GFP_NOFS);
		if (!crypt_info->ci_key) {
			res = -ENOMEM;
			goto out;
		}
		crypt_info->ci_key_len = keysize;
		/*lint -save -e732 -e747*/
		memcpy(crypt_info->ci_key, raw_key, crypt_info->ci_key_len);
		/*lint -restore*/
	}

	if (S_ISREG(inode->i_mode) &&
	    crypt_info->ci_data_mode == FS_ENCRYPTION_MODE_AES_128_CBC) {
		res = init_essiv_generator(crypt_info, raw_key, keysize);
		if (res) {
			pr_debug("%s: error %d (inode %lu) allocating essiv tfm\n",
				 __func__, res, inode->i_ino);
			goto out;
		}
	}
	if (cmpxchg(&inode->i_crypt_info, NULL, crypt_info) == NULL)
		crypt_info = NULL;
	fscrypt_set_verify_context(inode, &ctx, sizeof(ctx), NULL, has_crc);

out:
	if (res == -ENOKEY)
		res = 0;
	put_crypt_info(crypt_info);
	kzfree(raw_key);
	return res;
}
EXPORT_SYMBOL(fscrypt_get_encryption_info);

void fscrypt_put_encryption_info(struct inode *inode, struct fscrypt_info *ci)
{
	struct fscrypt_info *prev;

	if (ci == NULL)
		ci = ACCESS_ONCE(inode->i_crypt_info);
	if (ci == NULL)
		return;

	prev = cmpxchg(&inode->i_crypt_info, ci, NULL);
	if (prev != ci)
		return;

	put_crypt_info(ci);
}
EXPORT_SYMBOL(fscrypt_put_encryption_info);
