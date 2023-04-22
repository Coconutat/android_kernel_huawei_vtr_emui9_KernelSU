/*
 * Encryption policy functions for per-f2fs sdp file encryption support.
 *
 * Copyright (C) 2017, Huawei, Ltd..
 *
 * Written by Li minfeng and Luo Peng 2018.
 *
 */

#ifndef __F2FS_SDP_H__
#define __F2FS_SDP_H__

#include <linux/fscrypt_common.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <crypto/kpp.h>
#include <crypto/ecdh.h>
#include <linux/f2fs_fs.h>
#include "f2fs.h"

#if DEFINE_F2FS_FS_SDP_ENCRYPTION

#define DEBUG_F2FS_FS_SDP_ENCRYPTION  0
#if DEBUG_F2FS_FS_SDP_ENCRYPTION
#define pr_sdp_info pr_info
#else
#define pr_sdp_info(fmt, ...) \
	no_printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#endif

/*means xattr sdp ece crypt is enabled*/
#define F2FS_XATTR_SDP_ECE_ENABLE_FLAG           (1)
/*means xattr sdp ece crypt is be config, but may not be enabled*/
#define F2FS_XATTR_SDP_ECE_CONFIG_FLAG           (2)
/*means xattr sdp sece crypt is enabled*/
#define F2FS_XATTR_SDP_SECE_ENABLE_FLAG          (4)
/*means xattr sdp sece crypt is be config, but may not be enabled*/
#define F2FS_XATTR_SDP_SECE_CONFIG_FLAG          (8)

#define F2FS_INODE_IS_CONFIG_SDP_ECE_ENCRYPTION(flag)    \
	((flag) & (F2FS_XATTR_SDP_ECE_CONFIG_FLAG))
#define F2FS_INODE_IS_CONFIG_SDP_SECE_ENCRYPTION(flag)   \
	((flag) & (F2FS_XATTR_SDP_SECE_CONFIG_FLAG))
#define F2FS_INODE_IS_CONFIG_SDP_ENCRYPTION(flag)        \
	(((flag) & (F2FS_XATTR_SDP_SECE_CONFIG_FLAG))    \
	 || ((flag) & (F2FS_XATTR_SDP_ECE_CONFIG_FLAG)))
#define F2FS_INODE_IS_ENABLED_SDP_ECE_ENCRYPTION(flag)   \
	((flag) & (F2FS_XATTR_SDP_ECE_ENABLE_FLAG))
#define F2FS_INODE_IS_ENABLED_SDP_SECE_ENCRYPTION(flag)  \
	((flag) & (F2FS_XATTR_SDP_SECE_ENABLE_FLAG))
#define F2FS_INODE_IS_ENABLED_SDP_ENCRYPTION(flag)       \
	(((flag) & (F2FS_XATTR_SDP_ECE_ENABLE_FLAG))     \
	 || ((flag) & (F2FS_XATTR_SDP_SECE_ENABLE_FLAG)))

/** for sdp fscrypt policy and node xattr
 */
#define FSCRYPT_INVALID_CLASS	  (0)
#define FSCRYPT_CE_CLASS          (1)
#define FSCRYPT_SDP_ECE_CLASS     (2)
#define FSCRYPT_SDP_SECE_CLASS    (3)

#define FS_KEY_DERIVATION_PUBKEY_SIZE	(64)
#define FS_AES_256_GCM_KEY_SIZE		(32)
#define FS_AES_256_CBC_KEY_SIZE		(32)
#define FS_AES_256_CTS_KEY_SIZE		(32)
#define FS_AES_256_XTS_KEY_SIZE		(64)
#define FS_ENCRYPTION_CONTEXT_FORMAT_V2	(2)

#define FS_KEY_INDEX_OFFSET		(63)

struct fscrypt_sdp_key {
	u32 version;
	u32 sdpclass;
	u32 mode;
	u8 raw[FS_MAX_KEY_SIZE];
	u32 size;
	u8 pubkey[FS_MAX_KEY_SIZE];
	u32 pubkeysize;
} __packed;

struct f2fs_sdp_fscrypt_context {
	u8 version;
	u8 sdpclass;
	u8 format;
	u8 contents_encryption_mode;
	u8 filenames_encryption_mode;
	u8 flags;
	u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
	u8 nonce[FS_KEY_DERIVATION_CIPHER_SIZE];
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
	u8 file_pub_key[FS_KEY_DERIVATION_PUBKEY_SIZE];
} __packed;

/** policy.c internal functions
 */
int f2fs_inode_get_sdp_encrypt_flags(struct inode *inode, void *fs_data, u32 *flag);
int f2fs_inode_set_enabled_sdp_ece_encryption_flags(struct inode *inode,
	void *fs_data);
int f2fs_inode_set_enabled_sdp_sece_encryption_flags(struct inode *inode,
	void *fs_data);
int f2fs_inode_check_policy_keyring(u8 *descriptor, u8 sdpclass,
	u8 enforce_check);

/** keyinfo.c internal functions
 */
int f2fs_change_to_sdp_crypto(struct inode *inode, void *fs_data);

/** ecdh.c internal functions
 */

/**
 * get_file_pubkey_shared_secret() - Use ECDH to generate file public key
 * and shared secret.
 *
 * @cuive_id:          Curve id, only ECC_CURVE_NIST_P256 now
 * @dev_pub_key:       Device public key
 * @dev_pub_key_len:   The length of @dev_pub_key in bytes
 * @file_pub_key[out]: File public key to be generated
 * @file_pub_key_len:  The length of @file_pub_key in bytes
 * @shared secret[out]:Compute shared secret with file private key
 * and device public key
 * @shared_secret_len: The length of @shared_secret in bytes
 *
 * Return: 0 for success, error code in case of error.
 *         The contents of @file_pub_key and @shared_secret are
 *         undefined in case of error.
 */
int get_file_pubkey_shared_secret(unsigned int curve_id,
	const u8     *dev_pub_key,
	unsigned int dev_pub_key_len,
	u8           *file_pub_key,
	unsigned int file_pub_key_len,
	u8           *shared_secret,
	unsigned int shared_secret_len);

/**
 * get_shared_secret() - Use ECDH to generate shared secret.
 *
 * @cuive_id:          Curve id, only ECC_CURVE_NIST_P256 now
 * @dev_privkey:       Device private key
 * @dev_privkey_len:   The length of @dev_privkey in bytes
 * @file_pub_key:      File public key
 * @file_pub_key_len:  The length of @file_pub_key in bytes
 * @shared secret[out]:Compute shared secret with file public key and device
 *                     private key
 * @shared_secret_len: The length of @shared_secret in bytes
 *
 * Return: 0 for success, error code in case of error.
 *         The content of @shared_secret is undefined in case of error.
 */
int get_shared_secret(unsigned int curve_id,
	const u8     *dev_privkey,
	unsigned int dev_privkey_len,
	const u8     *file_pub_key,
	unsigned int file_pub_key_len,
	u8           *shared_secret,
	unsigned int shared_secret_len);

#endif
#endif

