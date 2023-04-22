/*
 * Encryption policy functions for per-f2fs sdp file encryption support.
 *
 * Copyright (C) 2017, Huawei, Ltd..
 *
 * Written by Luo Peng, 2017.
 *
 */

#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <linux/printk.h>
#include "f2fs_sdp.h"

#if DEFINE_F2FS_FS_SDP_ENCRYPTION
struct tcrypt_result {
	struct completion completion;
	int err;
};

static void tcrypt_complete(struct crypto_async_request *req, int err)
{
	struct tcrypt_result *res = req->data;

	if (err == -EINPROGRESS)
		return;

	res->err = err;
	complete(&res->completion);
}

static int wait_async_op(int ret, struct tcrypt_result *tr)
{
	if (ret == -EINPROGRESS || ret == -EBUSY) {
		wait_for_completion(&tr->completion);
		reinit_completion(&tr->completion);
		ret = tr->err;
	}
	return ret;
}

/**
 * ecdh_gen_filepubkey_secret() - Generate file public key and shared secret
 *
 * @tfm          : Kpp transformation with ecdh algorithm
 * @curve_id     : Curve id, ECC_CURVE_NIST_P256 now
 * @dev_pub_key  : Device public key. Get from keyring.
 * @file_pub_key[out] : Generated file public key. To be stored in file xattr.
 * @shared_secret[out]: Used to decrypt the file content encryption key in GCM
 *               mode. Not stored.
 */
static int ecdh_gen_filepubkey_secret(struct crypto_kpp *tfm,
	int      curve_id,
	const u8 *dev_pub_key,
	u8       *file_pub_key,
	u8       *shared_secret)
{
	struct kpp_request   *req;
	struct ecdh          *ecdh_param;
	void                 *ecdh_buf   = NULL;
	void                 *output_buf = NULL;
	void                 *input_buf  = NULL;
	unsigned int         ecdh_size;
	unsigned int         pub_key_size;
	unsigned int         privkey_len;
	struct tcrypt_result result;
	int                  err         = -ENOMEM;
	struct scatterlist   src, dst;

	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return err;

	init_completion(&result.completion);

	/* Set the ecdh params */
	ecdh_param = kzalloc(sizeof(struct ecdh), GFP_KERNEL);
	if (!ecdh_param)
		goto free_req;

	ecdh_param->curve_id = curve_id;
	ecdh_param->key      = NULL;
	ecdh_param->key_size = 0;
	ecdh_size            = crypto_ecdh_key_len(ecdh_param);
	ecdh_buf             = kzalloc(ecdh_size, GFP_KERNEL);
	if (!ecdh_buf)
		goto free_ecdh_param;

	err = crypto_ecdh_encode_key(ecdh_buf, ecdh_size, ecdh_param);
	if (err < 0)
		goto free_ecdh_buf;

	/* Generate file private key */
	err = crypto_kpp_set_secret(tfm, ecdh_buf, ecdh_size);
	if (err < 0)
		goto free_ecdh_buf;

	/* Compute and store file public key */
	pub_key_size = crypto_kpp_maxsize(tfm);
	privkey_len  = pub_key_size >> 1;
	output_buf   = kzalloc(pub_key_size, GFP_KERNEL);
	if (!output_buf) {
		err = -ENOMEM;
		goto free_ecdh_buf;
	}

	/* File private key has been in the kpp request.
	 * No other input is needed for generating public key.
	 */
	kpp_request_set_input(req, NULL, 0);
	sg_init_one(&dst, output_buf, pub_key_size);
	kpp_request_set_output(req, &dst, pub_key_size);
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				 tcrypt_complete, &result);

	err = wait_async_op(crypto_kpp_generate_public_key(req), &result);
	if (err) {
		pr_err("Alg: %s: generates file public key failed. err %d\n",
		       (tfm->base.__crt_alg)->cra_name, err);
		goto free_output;
	}
	memcpy(file_pub_key, sg_virt(req->dst), pub_key_size);

	/*Calculate shared secret with file private key and device public key*/
	input_buf = kzalloc(pub_key_size, GFP_KERNEL);
	if (!input_buf) {
		err = -ENOMEM;
		goto free_output;
	}
	memcpy(input_buf, dev_pub_key, pub_key_size);
	sg_init_one(&src, input_buf, pub_key_size);
	sg_init_one(&dst, output_buf, privkey_len);
	kpp_request_set_input(req, &src, pub_key_size);
	kpp_request_set_output(req, &dst, privkey_len);
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				 tcrypt_complete, &result);
	err = wait_async_op(crypto_kpp_compute_shared_secret(req), &result);
	if (err) {
		pr_err("Alg: %s: computes shared secret failed in ecdh_gen_filepubkey_secret. err %d\n",
		       (tfm->base.__crt_alg)->cra_name, err);
		goto free_all;
	}

	memcpy(shared_secret, sg_virt(req->dst), privkey_len);

free_all:
	kzfree(input_buf);
free_output:
	kzfree(output_buf);
free_ecdh_buf:
	kzfree(ecdh_buf);
free_ecdh_param:
	kzfree(ecdh_param);
free_req:
	kpp_request_free(req);
	input_buf  = NULL;
	output_buf = NULL;
	ecdh_buf   = NULL;
	return err;
}

/**
 * ecdh_gen_secret() - Generate shared secret
 *
 * @tfm          : Kpp transformation with ecdh algorithm
 * @curve_id     : Curve id, only ECC_CURVE_NIST_P256 now
 * @dev_privkey  : Device private key which is gotten from keyring.
 * @file_pub_key : File public key which is gotten from file xattr.
 * @shared_secret[out]: Used to decrypt the file content encryption key in GCM
 *               mode. Not stored.
 */
static int ecdh_gen_secret(struct crypto_kpp *tfm,
	unsigned int curve_id,
	const u8     *dev_privkey,
	const u8     *file_pub_key,
	u8           *shared_secret)
{
	struct kpp_request   *req;
	struct ecdh          *ecdh_param;
	void                 *ecdh_buf   = NULL;
	void                 *output_buf = NULL;
	void                 *input_buf  = NULL;
	unsigned int         ecdh_size;
	unsigned int         pub_key_size;
	unsigned int         privkey_len;
	struct tcrypt_result result;
	int                  err         = -ENOMEM;
	struct scatterlist   src, dst;
	u8                   dev_privkey_tmp[32];

	req = kpp_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return err;

	init_completion(&result.completion);

	/* Set the ecdh params */
	ecdh_param = kzalloc(sizeof(struct ecdh), GFP_KERNEL);
	if (!ecdh_param)
		goto free_req;

	ecdh_param->curve_id = curve_id;
	ecdh_param->key_size = 32;
	memcpy(dev_privkey_tmp, dev_privkey, 32);
	ecdh_param->key      = dev_privkey_tmp;
	ecdh_size            = crypto_ecdh_key_len(ecdh_param);
	ecdh_buf             = kzalloc(ecdh_size, GFP_KERNEL);
	if (!ecdh_buf)
		goto free_ecdh_param;

	err = crypto_ecdh_encode_key(ecdh_buf, ecdh_size, ecdh_param);
	if (err < 0)
		goto free_ecdh_buf;

	/* Set the device private key */
	err = crypto_kpp_set_secret(tfm, ecdh_buf, ecdh_size);
	if (err < 0)
		goto free_ecdh_buf;

	/* Malloc for file public key (input) and shared secret (output) */
	pub_key_size = crypto_kpp_maxsize(tfm);
	privkey_len  = pub_key_size >> 1;
	output_buf   = kzalloc(privkey_len, GFP_KERNEL);
	if (!output_buf) {
		err  = -ENOMEM;
		goto free_ecdh_buf;
	}
	input_buf    = kzalloc(pub_key_size, GFP_KERNEL);
	if (!input_buf) {
		err  = -ENOMEM;
		goto free_output;
	}

	/*Calculate shared secret with device private key and file public key*/
	memcpy(input_buf, file_pub_key, pub_key_size);
	sg_init_one(&src, input_buf, pub_key_size);
	sg_init_one(&dst, output_buf, privkey_len);
	kpp_request_set_input(req, &src, pub_key_size);
	kpp_request_set_output(req, &dst, privkey_len);
	kpp_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				 tcrypt_complete, &result);
	err = wait_async_op(crypto_kpp_compute_shared_secret(req), &result);
	if (err) {
		pr_err("Alg: %s: computes shared secret failed in ecdh_gen_secret. err %d\n",
		       (tfm->base.__crt_alg)->cra_name, err);
		goto free_all;
	}

	/* store shared secret */
	memcpy(shared_secret, sg_virt(req->dst), privkey_len);

free_all:
	kzfree(input_buf);
free_output:
	kzfree(output_buf);
free_ecdh_buf:
	kzfree(ecdh_buf);
free_ecdh_param:
	kzfree(ecdh_param);
free_req:
	kpp_request_free(req);
	memset(dev_privkey_tmp, 0x00, 32);
	input_buf  = NULL;
	output_buf = NULL;
	ecdh_buf   = NULL;
	return err;
}

int get_file_pubkey_shared_secret(unsigned int curve_id,
	const u8     *dev_pub_key,
	unsigned int dev_pub_key_len,
	u8           *file_pub_key,
	unsigned int file_pub_key_len,
	u8           *shared_secret,
	unsigned int shared_secret_len)
{
	const char        *alg_name = "ecdh";
	struct crypto_kpp *tfm;
	int               err       = -ENOMEM;

	if ((curve_id != ECC_CURVE_NIST_P256) ||
	    (dev_pub_key_len != 64) ||
	    (file_pub_key_len != 64) ||
	    (shared_secret_len != 32))
		return -EINVAL;

	tfm = crypto_alloc_kpp(alg_name, CRYPTO_ALG_TYPE_KPP,
			       CRYPTO_ALG_TYPE_MASK);
	if (IS_ERR(tfm)) {
		pr_err("Alg: kpp: Failed to load tfm for %s: %ld\n",
		       alg_name, PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	err = ecdh_gen_filepubkey_secret(tfm, curve_id, dev_pub_key,
					 file_pub_key, shared_secret);
	if (err)
		pr_err("Alg: kpp: Failed to generate file public key and shared secret.\n");

	crypto_free_kpp(tfm);
	return err;
}

int get_shared_secret(unsigned int curve_id,
	const u8     *dev_privkey,
	unsigned int dev_privkey_len,
	const u8     *file_pub_key,
	unsigned int file_pub_key_len,
	u8           *shared_secret,
	unsigned int shared_secret_len)
{
	const char        *alg_name = "ecdh";
	struct crypto_kpp *tfm;
	int               err       = -ENOMEM;

	if ((curve_id != ECC_CURVE_NIST_P256) ||
	    (dev_privkey_len != 32) ||
	    (file_pub_key_len != 64) ||
	    (shared_secret_len != 32))
		return -EINVAL;

	tfm = crypto_alloc_kpp(alg_name, CRYPTO_ALG_TYPE_KPP,
			       CRYPTO_ALG_TYPE_MASK);
	if (IS_ERR(tfm)) {
		pr_err("Alg: kpp: Failed to load tfm for %s: %ld\n",
		       alg_name, PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	err = ecdh_gen_secret(tfm, curve_id, dev_privkey, file_pub_key,
			      shared_secret);
	if (err)
		pr_err("Alg: kpp: Failed to generate shared secret.\n");

	crypto_free_kpp(tfm);
	return err;
}
#endif
