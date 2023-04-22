/* SPDX-License-Identifier: GPL-2.0 */

/* should be avoid in the future */
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 31))
__SETPAGEFLAG(Referenced, referenced)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0))
#define d_inode(d) ((d)->d_inode)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0))
#define d_really_is_negative(d) (d_inode(d) == NULL)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
/* Restricts the given gfp_mask to what the mapping allows. */
static inline gfp_t mapping_gfp_constraint(
	struct address_space *mapping,
	gfp_t gfp_mask)
{
	return mapping_gfp_mask(mapping) & gfp_mask;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 116))
static inline void inode_nohighmem(struct inode *inode)
{
	mapping_set_gfp_mask(inode->i_mapping, GFP_USER);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0))

/* bio stuffs */
#define REQ_OP_READ    READ
#define REQ_OP_WRITE   WRITE
#define bio_op(bio)    ((bio)->bi_rw & 1)

static inline void bio_set_op_attrs(struct bio *bio,
	unsigned op, unsigned op_flags) {
	bio->bi_rw = op | op_flags;
}

static inline gfp_t readahead_gfp_mask(struct address_space *x)
{
	return mapping_gfp_mask(x) |  __GFP_COLD |
	                              __GFP_NORETRY | __GFP_NOWARN;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 13))
#define READ_ONCE(x)		ACCESS_ONCE(x)
#define WRITE_ONCE(x, val)	(ACCESS_ONCE(x) = (val))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 40))
static inline int lockref_put_return(struct lockref *lockref)
{
	return -1;
}
#endif

#ifndef WQ_NON_REENTRANT
#define WQ_NON_REENTRANT 0
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0))
#define page_cache_get(page)            get_page(page)
#define page_cache_release(page)        put_page(page)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
static inline bool sb_rdonly(const struct super_block *sb) {
	return sb->s_flags & MS_RDONLY;
}

#define bio_set_dev(bio, bdev)	((bio)->bi_bdev = (bdev))

#endif

#ifndef lru_to_page
#define lru_to_page(_head) (list_entry((_head)->prev, struct page, lru))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))

static inline void *kvmalloc(size_t size, gfp_t flags)
{
	void *buffer = NULL;

	if (size == 0)
		return NULL;

	/* do not attempt kmalloc if we need more than 16 pages at once */
	if (size <= (16 * PAGE_SIZE))
		buffer = kmalloc(size, flags);
	if (!buffer) {
		if (flags & __GFP_ZERO)
			buffer = vzalloc(size);
		else
			buffer = vmalloc(size);
	}
	return buffer;
}

static inline void *kvzalloc(size_t size, gfp_t flags)
{
	return kvmalloc(size, flags | __GFP_ZERO);
}

static inline void *kvmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;

	return kvmalloc(n * size, flags);
}

#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 15, 0))
static inline void kvfree(const void *addr)
{
	if (is_vmalloc_addr(addr))
		vfree(addr);
	else
		kfree(addr);
}
#endif

