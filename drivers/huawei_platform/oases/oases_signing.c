/*
 * oases_signing.c - init key and verify signature
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/err.h>

#include "util.h"
#include "oases_signing.h"
#include "sign/x509_parser.h"

struct patch_signature {
	u8	algo;		/* Public-key crypto algorithm [enum pkey_algo] */
	u8	hash;		/* Digest algorithm [enum pkey_hash_algo] */
	u8	id_type;	/* Key identifier type [enum pkey_id_type] */
	u8	signer_len;	/* Length of signer's name */
	u8	key_id_len;	/* Length of key identifier */
	u8	__pad[3];
	__be32	sig_len;	/* Length of signature data */
};

extern __initdata const u8 oases_sign_certificate_list[];
extern __initdata const u8 oases_sign_certificate_list_end[];

extern __initdata const u8 vendor_sign_certificate_list[];
extern __initdata const u8 vendor_sign_certificate_list_end[];

static struct x509_certificate *oases_cert = NULL;
static struct x509_certificate *vendor_cert = NULL;


extern struct shash_alg oases_sha512_algs;
extern int oases_sha512_init(struct shash_desc *desc);
extern int oases_crypto_sha512_update(struct shash_desc *desc, const u8 *data,
		unsigned int len);
extern int oases_sha512_final(struct shash_desc *desc, u8 *hash);
extern int oases_RSA_verify_signature(const struct public_key *key,
		const struct public_key_signature *sig);
extern struct x509_certificate *oases_x509_cert_parse(const void *data, size_t datalen);
extern void oases_x509_free_certificate(struct x509_certificate *cert);

static struct x509_certificate *oases_key_create(const u8 *liststart, const u8 *listend)
{
	struct x509_certificate *cert;
	const u8 *p, *end;
	size_t plen;

	p = liststart;
	end = listend;
	while (p < end) {
		/* Each cert begins with an ASN.1 SEQUENCE tag and must be more
		 * than 256 bytes in size.
		 */
		if (end - p < 4)
			goto fail;
		if (p[0] != 0x30 &&
		    p[1] != 0x82)
			goto fail;
		plen = (p[2] << 8) | p[3];
		plen += 4;
		if (plen > end - p)
			goto fail;

		cert = oases_x509_cert_parse(p, plen);
		if (IS_ERR(cert)) {
			oases_error("loading X.509 certificate (%ld) fail\n",
					PTR_ERR(cert));
			goto fail;
		} else {
			return cert;
		}
		p += plen;
	}

fail:
	return ERR_PTR(-ENOKEY);
}

int __init oases_init_signing_keys(void)
{
	struct x509_certificate *cert;

	cert = oases_key_create(oases_sign_certificate_list,
				oases_sign_certificate_list_end);
	if (IS_ERR(cert)) {
		oases_debug("loaded oases cert fail\n");
		return -ENOKEY;
	} else {
		oases_cert = cert;
	}

	cert = oases_key_create(vendor_sign_certificate_list,
				vendor_sign_certificate_list_end);
	if (IS_ERR(cert)) {
		oases_debug("loaded vendor cert fail\n");
		goto failed_free_cert;
	} else {
		vendor_cert = cert;
	}

	return 0;

failed_free_cert:
	oases_x509_free_certificate(oases_cert);
	return -ENOKEY;
}

void oases_destroy_signing_keys(void)
{
	if (oases_cert)
		oases_x509_free_certificate(oases_cert);
	if (vendor_cert)
		oases_x509_free_certificate(vendor_cert);
}

static struct public_key_signature *oases_make_digest(
	enum pkey_hash_algo hash, const void *patch, unsigned long patchlen)
{
	struct public_key_signature *pks;
	struct shash_desc *desc;
	size_t digest_size, desc_size;

	/* Allocate the hashing algorithm we're going to need and find out how
	 * big the hash operational data will be.
	 */

	desc_size = oases_sha512_algs.descsize + sizeof(*desc);
	digest_size = oases_sha512_algs.digestsize;

	/* We allocate the hash operational data storage on the end of our
	 * context data and the digest output buffer on the end of that.
	 */
	pks = kzalloc(digest_size + sizeof(*pks) + desc_size, GFP_KERNEL);
	if (!pks)
		return ERR_PTR(-ENOMEM);

	pks->pkey_hash_algo	= hash;
	pks->digest		= (u8 *)pks + sizeof(*pks) + desc_size;
	pks->digest_size	= digest_size;

	desc = (void *)pks + sizeof(*pks);
	desc->flags = CRYPTO_TFM_REQ_MAY_SLEEP;

	oases_sha512_init(desc);
	oases_crypto_sha512_update(desc, patch, patchlen);
	oases_sha512_final(desc, pks->digest);

	return pks;
}

/*
 * Extract an MPI array from the signature data.  This represents the actual
 * signature.  Each raw MPI is prefaced by a BE 2-byte value indicating the
 * size of the MPI in bytes.
 *
 * RSA signatures only have one MPI, so currently we only read one.
 */
static int oases_extract_mpi_array(struct public_key_signature *pks,
				 const void *data, size_t len)
{
	size_t nbytes;
	MPI mpi;

	if (len < 3)
		return -EBADMSG;
	nbytes = ((const u8 *)data)[0] << 8 | ((const u8 *)data)[1];
	data += 2;
	len -= 2;
	if (len != nbytes)
		return -EBADMSG;

	mpi = oases_mpi_read_raw_data(data, nbytes);
	if (!mpi)
		return -ENOMEM;
	pks->mpi[0] = mpi;
	pks->nr_mpi = 1;
	return 0;
}

int oases_verify_sig(char *data, unsigned long *_patchlen, int sig_type)
{
	struct patch_signature ps;
	size_t patchlen = *_patchlen;
	size_t sig_len;
	struct public_key_signature *pks;
	int ret;
	struct public_key *key;
	const void *sig;
	if (patchlen <= sizeof(ps))
		return -EBADMSG;

	memcpy(&ps, data + (patchlen - sizeof(ps)), sizeof(ps));
	patchlen -= sizeof(ps);

	sig_len = be32_to_cpu(ps.sig_len);
	if (sig_len >= patchlen)
		return -EBADMSG;
	patchlen -= sig_len;
	if (((size_t)ps.signer_len + (size_t)ps.key_id_len) >= patchlen)
		return -EBADMSG;
	patchlen -= (size_t)ps.signer_len + (size_t)ps.key_id_len;
	*_patchlen = patchlen;
	sig = data + patchlen;
	if (ps.algo != PKEY_ALGO_RSA || ps.id_type != PKEY_ID_X509) {
		oases_debug("key alg or id type error\n");
		return -ENOPKG;
	}

	if (ps.hash >= PKEY_HASH__LAST || !oases_pkey_hash_algo[ps.hash]) {
		oases_debug("hash type error error\n");
		return -ENOPKG;
	}

	pks = oases_make_digest(ps.hash, data, patchlen);
	if (IS_ERR(pks)) {
		ret = PTR_ERR(pks);
		goto out;
	}
	ret = oases_extract_mpi_array(pks, sig + ps.signer_len + ps.key_id_len,
					sig_len);
	if (ret < 0)
		goto error_free_pks;

	if (sig_type == SIG_TYPE_VENDOR)
		key = vendor_cert->pub;
	else
		key = oases_cert->pub;

	ret = oases_RSA_verify_signature(key, pks);
	if (!ret)
		oases_debug("oases_verify_sig sucess\n");

error_free_pks:
	oases_mpi_free(pks->rsa.s);
	kfree(pks);
out:
	return ret;
}
