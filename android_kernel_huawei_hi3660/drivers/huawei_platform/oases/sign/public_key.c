/* In-software asymmetric public-key crypto subtype
 *
 * See Documentation/crypto/asymmetric-keys.txt
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/slab.h>

#include "public_key.h"

const char *const oases_pkey_algo[PKEY_ALGO__LAST] = {
	[PKEY_ALGO_DSA] = "DSA",
	[PKEY_ALGO_RSA] = "RSA",
};

const char *const oases_pkey_hash_algo[PKEY_HASH__LAST] = {
	[PKEY_HASH_MD4] = "md4",
	[PKEY_HASH_MD5] = "md5",
	[PKEY_HASH_SHA1] = "sha1",
	[PKEY_HASH_RIPE_MD_160]	= "rmd160",
	[PKEY_HASH_SHA256] = "sha256",
	[PKEY_HASH_SHA384] = "sha384",
	[PKEY_HASH_SHA512] = "sha512",
	[PKEY_HASH_SHA224] = "sha224",
};

const char *const oases_pkey_id_type[PKEY_ID_TYPE__LAST] = {
	[PKEY_ID_PGP] = "PGP",
	[PKEY_ID_X509] = "X509",
};

/*
 * Destroy a public key algorithm key.
 */
void oases_public_key_destroy(void *payload)
{
	struct public_key *key = payload;
	int i;

	if (key) {
		for (i = 0; i < ARRAY_SIZE(key->mpi); i++)
			oases_mpi_free(key->mpi[i]);
		kfree(key);
	}
}
