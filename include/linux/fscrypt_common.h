/*
 * fscrypt_common.h: common declarations for per-file encryption
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * Written by Michael Halcrow, 2015.
 * Modified by Jaegeuk Kim, 2015.
 */

#ifndef _LINUX_FSCRYPT_COMMON_H
#define _LINUX_FSCRYPT_COMMON_H

#include <linux/key.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/dcache.h>
#include <crypto/skcipher.h>
#include <crypto/aead.h>
#include <uapi/linux/fs.h>

#define FS_CRYPTO_BLOCK_SIZE		16

#define FS_KEY_DERIVATION_NONCE_SIZE		64
#define FS_KEY_DERIVATION_IV_SIZE		16
#define FS_KEY_DERIVATION_CIPHER_SIZE		(64 + 16) /* nonce + tag */

/**
 * Encryption context for inode
 *
 * Protector format:
 *  1 byte: Protector format (2 = this version)
 *  1 byte: File contents encryption mode
 *  1 byte: File names encryption mode
 *  1 byte: Flags
 *  8 bytes: Master Key descriptor
 *  80 bytes: Encryption Key derivation nonce (encrypted)
 *  12 bytes: IV
 */
struct fscrypt_context {
	u8 format;
	u8 contents_encryption_mode;
	u8 filenames_encryption_mode;
	u8 flags;
	u8 master_key_descriptor[FS_KEY_DESCRIPTOR_SIZE];
	u8 nonce[FS_KEY_DERIVATION_CIPHER_SIZE];
	u8 iv[FS_KEY_DERIVATION_IV_SIZE];
} __packed;

/*
 * A pointer to this structure is stored in the file system's in-core
 * representation of an inode.
 */
struct fscrypt_info {
	u8 ci_data_mode;
	u8 ci_filename_mode;
	u8 ci_flags;
	struct crypto_skcipher *ci_ctfm;
	struct crypto_aead *ci_gtfm;
	struct crypto_cipher *ci_essiv_tfm;
	u8 ci_master_key[FS_KEY_DESCRIPTOR_SIZE];
	void *ci_key;
	int ci_key_len;
	int ci_key_index;
	u8  ci_sdp_flag;
};

static inline void *fscrypt_ci_key(struct inode *inode)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key;
#else
	return NULL;
#endif
}

static inline int fscrypt_ci_key_len(struct inode *inode)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key_len;
#else
	return 0;
#endif
}

static inline int fscrypt_ci_key_index(struct inode *inode)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
	return inode->i_crypt_info->ci_key_index;
#else
	return -1;
#endif
}

struct fscrypt_ctx {
	union {
		struct {
			struct page *bounce_page;	/* Ciphertext page */
			struct page *control_page;	/* Original page  */
		} w;
		struct {
			struct bio *bio;
			struct work_struct work;
		} r;
		struct list_head free_list;	/* Free list */
	};
	u8 flags;				/* Flags */
};

/**
 * For encrypted symlinks, the ciphertext length is stored at the beginning
 * of the string in little-endian format.
 */
struct fscrypt_symlink_data {
	__le16 len;
	char encrypted_path[1];
} __packed;

struct fscrypt_str {
	unsigned char *name;
	u32 len;
};

struct fscrypt_name {
	const struct qstr *usr_fname;
	struct fscrypt_str disk_name;
	u32 hash;
	u32 minor_hash;
	struct fscrypt_str crypto_buf;
};

#define FSTR_INIT(n, l)		{ .name = n, .len = l }
#define FSTR_TO_QSTR(f)		QSTR_INIT((f)->name, (f)->len)
#define fname_name(p)		((p)->disk_name.name)
#define fname_len(p)		((p)->disk_name.len)

/*
 * fscrypt superblock flags
 */
#define FS_CFLG_OWN_PAGES (1U << 1)

/*
 * crypto opertions for filesystems
 */
struct fscrypt_operations {
	unsigned int flags;
	const char *key_prefix;
	int (*get_context)(struct inode *, void *, size_t, int *);
	int (*get_verify_context)(struct inode *, void *, size_t);
	int (*set_context)(struct inode *, const void *, size_t, void *);
	int (*set_verify_context)(struct inode *, const void *, size_t,
				  void *, int);
	int (*dummy_context)(struct inode *);
	bool (*is_encrypted)(struct inode *);
	bool (*is_inline_encrypted)(struct inode *);
	void (*set_encrypted_corrupt)(struct inode *);
	bool (*is_encrypted_fixed)(struct inode *);
	bool (*empty_dir)(struct inode *);
	unsigned (*max_namelen)(struct inode *);
	int (*get_keyinfo)(struct inode *, void *, int *);
	int (*is_permitted_context)(struct inode *, struct inode *);
};

static inline bool fscrypt_dummy_context_enabled(struct inode *inode)
{
	if (inode->i_sb->s_cop->dummy_context &&
				inode->i_sb->s_cop->dummy_context(inode))
		return true;
	return false;
}

static inline bool fscrypt_valid_enc_modes(u32 contents_mode,
					u32 filenames_mode)
{
	if (contents_mode == FS_ENCRYPTION_MODE_AES_128_CBC &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_128_CTS)
		return true;

	if (contents_mode == FS_ENCRYPTION_MODE_AES_256_XTS &&
	    filenames_mode == FS_ENCRYPTION_MODE_AES_256_CTS)
		return true;

	return false;
}

static inline bool fscrypt_is_dot_dotdot(const struct qstr *str)
{
	if (str->len == 1 && str->name[0] == '.')
		return true;

	if (str->len == 2 && str->name[0] == '.' && str->name[1] == '.')
		return true;

	return false;
}

static inline struct page *fscrypt_control_page(struct page *page)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
	return ((struct fscrypt_ctx *)page_private(page))->w.control_page;
#else
	WARN_ON_ONCE(1);
	return ERR_PTR(-EINVAL);
#endif
}

static inline int fscrypt_has_encryption_key(const struct inode *inode)
{
#if IS_ENABLED(CONFIG_FS_ENCRYPTION)
	return (inode->i_crypt_info != NULL);
#else
	return 0;
#endif
}

#endif	/* _LINUX_FSCRYPT_COMMON_H */
