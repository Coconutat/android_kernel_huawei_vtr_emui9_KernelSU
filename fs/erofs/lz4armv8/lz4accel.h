#include <linux/types.h>
#include <asm/simd.h>

#if defined(CONFIG_ARM64) && defined(CONFIG_KERNEL_MODE_NEON)
#include <asm/neon.h>
extern int _lz4_decompress_asm(uint8_t **dst_ptr, uint8_t *dst_begin,
			uint8_t *dst_end, const uint8_t **src_ptr,
			const uint8_t *src_end);

static inline int lz4_decompress_accel_enable(void)
{
	return	may_use_simd();
}

static inline ssize_t lz4_decompress_asm(
	uint8_t **dst_ptr, uint8_t *dst_begin, uint8_t *dst_end,
	const uint8_t **src_ptr, const uint8_t *src_end)
{
	int ret;
	kernel_neon_begin();
	ret = _lz4_decompress_asm(dst_ptr, dst_begin, dst_end, src_ptr, src_end);
	kernel_neon_end();

	return (ssize_t)ret;
}

#define __ARCH_HAS_LZ4_ACCELERATOR

#else

static inline int lz4_decompress_accel_enable(void)
{
	return	0;
}

static inline ssize_t lz4_decompress_asm(
	uint8_t **dst_ptr, uint8_t *dst_begin, uint8_t *dst_end,
	const uint8_t **src_ptr, const uint8_t *src_end)
{
	return 0;
}
#endif
