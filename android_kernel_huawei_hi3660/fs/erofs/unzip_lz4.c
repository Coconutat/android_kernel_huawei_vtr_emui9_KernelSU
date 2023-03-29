// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * linux/drivers/staging/erofs/unzip_lz4.c
 *
 * Copyright (C) 2018 HUAWEI, Inc.
 *             http://www.huawei.com/
 * Created by Gao Xiang <gaoxiang25@huawei.com>
 *
 * Original code taken from 'linux/lib/lz4/lz4_decompress.c'
 */

/*
 * LZ4 - Fast LZ compression algorithm
 * Copyright (C) 2011 - 2016, Yann Collet.
 * BSD 2 - Clause License (http://www.opensource.org/licenses/bsd - license.php)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *	* Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 *	* Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * You can contact the author at :
 *	- LZ4 homepage : http://www.lz4.org
 *	- LZ4 source repository : https://github.com/lz4/lz4
 *
 *	Changed for kernel usage by:
 *	Sven Schmidt <4sschmid@informatik.uni-hamburg.de>
 */
#include "internal.h"
#include <asm/unaligned.h>
#include "lz4defs.h"
#include "lz4armv8/lz4accel.h"

#define LZ4_FAST_MARGIN                (128)

#ifndef SIZE_MAX
#define SIZE_MAX       (~(size_t)0)
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX      ((ssize_t)(SIZE_MAX >> 1))
#endif

/*
 * no public solution to solve our requirement yet.
 * see: <required buffer size for LZ4_decompress_safe_partial>
 *      https://groups.google.com/forum/#!topic/lz4c/_3kkz5N6n00
 */
static FORCE_INLINE ssize_t __customized_lz4_decompress_safe_partial(
	uint8_t         *dst_ptr,
	const uint8_t   *src_ptr,
	void            *dest,
	ssize_t         outputSize,
	const void      *source,
	ssize_t         inputSize)
{
	/* Local Variables */
	const BYTE * const iend = (const BYTE *) source + inputSize;
	BYTE * const oend = (BYTE *) dest + outputSize;

	const BYTE *ip = src_ptr;
	BYTE *op = dst_ptr;
	BYTE *cpy;

	static const unsigned int dec32table[] = { 0, 1, 2, 1, 4, 4, 4, 4 };
	static const int dec64table[] = { 0, 0, 0, -1, 0, 1, 2, 3 };

	/* Empty output buffer */
	if (unlikely(outputSize == 0))
		return ((inputSize == 1) && (*ip == 0)) ? 0 : -1;

	/* Main Loop : decode sequences */
	while (1) {
		size_t length;
		const BYTE *match;
		size_t offset;

		/* get literal length */
		unsigned int const token = *ip++;

		length = token>>ML_BITS;

		if (length == RUN_MASK) {
			unsigned int s;

			do {
				s = *ip++;
				length += s;
			} while ((ip < iend - RUN_MASK) & (s == 255));

			if (unlikely((size_t)(op + length) < (size_t)(op))) {
				/* overflow detection */
				goto _output_error;
			}
			if (unlikely((size_t)(ip + length) < (size_t)(ip))) {
				/* overflow detection */
				goto _output_error;
			}
		}

		/* copy literals */
		cpy = op + length;
		if ((cpy > oend - WILDCOPYLENGTH) ||
			(ip + length > iend - (2 + 1 + LASTLITERALS))) {
			if (cpy > oend) {
				memcpy(op, ip, length = oend - op);
				op += length;
				break;
			}

			if (unlikely(ip + length > iend)) {
				/*
				 * Error :
				 * read attempt beyond
				 * end of input buffer
				 */
				goto _output_error;
			}

			memcpy(op, ip, length);
			ip += length;
			op += length;

			if (ip > iend - 2)
				break;
			/* Necessarily EOF, due to parsing restrictions */
			/* break; */
		} else {
			LZ4_wildCopy(op, ip, cpy);
			ip += length;
			op = cpy;
		}

		/* get offset */
		offset = LZ4_readLE16(ip);
		ip += 2;
		match = op - offset;

		if (unlikely(match < (const BYTE *)dest)) {
			/* Error : offset outside buffers */
			goto _output_error;
		}

		/* get matchlength */
		length = token & ML_MASK;
		if (length == ML_MASK) {
			unsigned int s;

			do {
				s = *ip++;

				if (ip > iend - LASTLITERALS)
					goto _output_error;

				length += s;
			} while (s == 255);

			if (unlikely((size_t)(op + length) < (size_t)op)) {
				/* overflow detection */
				goto _output_error;
			}
		}

		length += MINMATCH;

		/* copy match within block */
		cpy = op + length;

		if (unlikely(cpy >= oend - WILDCOPYLENGTH)) {
			if (cpy >= oend) {
				while (op < oend)
					*op++ = *match++;
				break;
			}
			goto __match;
		}

		/* costs ~1%; silence an msan warning when offset == 0 */
		LZ4_write32(op, (U32)offset);

		if (unlikely(offset < 8)) {
			const int dec64 = dec64table[offset];

			op[0] = match[0];
			op[1] = match[1];
			op[2] = match[2];
			op[3] = match[3];
			match += dec32table[offset];
			memcpy(op + 4, match, 4);
			match -= dec64;
		} else {
			LZ4_copy8(op, match);
			match += 8;
		}

		op += 8;

		if (unlikely(cpy > oend - 12)) {
			BYTE * const oCopyLimit = oend - (WILDCOPYLENGTH - 1);

			if (op < oCopyLimit) {
				LZ4_wildCopy(op, match, oCopyLimit);
				match += oCopyLimit - op;
				op = oCopyLimit;
			}
__match:
			while (op < cpy)
				*op++ = *match++;
		} else {
			LZ4_copy8(op, match);

			if (length > 16)
				LZ4_wildCopy(op + 8, match + 8, cpy);
		}

		op = cpy; /* correction */
	}
	DBG_BUGON((void *)ip - source > inputSize);
	DBG_BUGON((void *)op - dest > outputSize);

	/* Nb of output bytes decoded */
	return (ssize_t) ((void *)op - dest);

	/* Overflow error detected */
_output_error:
	return -ERANGE;
}

static FORCE_INLINE int customized_lz4_decompress_safe_partial(
		void            *dest,
		ssize_t         outputSize,
		const void      *source,
		ssize_t         inputSize)
{
	uint8_t         *dstPtr = dest;
	const uint8_t   *srcPtr = source;
	ssize_t         ret;
	ret = __customized_lz4_decompress_safe_partial(dstPtr, srcPtr,
				dest, outputSize, source, inputSize);
	return ret;
}

#ifdef __ARCH_HAS_LZ4_ACCELERATOR
static FORCE_INLINE ssize_t lz4_decompress_accel(
	void            *dest,
	ssize_t         outputSize,
	const void      *source,
	ssize_t         inputSize)
{
	uint8_t         *dstPtr = dest;
	const uint8_t   *srcPtr = source;
	ssize_t         ret;
	/* Go fast if we can, keeping away from the end of buffers */
	if (outputSize > LZ4_FAST_MARGIN && inputSize > LZ4_FAST_MARGIN) {
	        ret = lz4_decompress_asm(&dstPtr, dest,
					dest + outputSize - LZ4_FAST_MARGIN,
					&srcPtr,
					source + inputSize - LZ4_FAST_MARGIN);
		if (ret)
			return 0;
	}

	/* Finish safe */
	ret = __customized_lz4_decompress_safe_partial(dstPtr, srcPtr,
				dest, outputSize, source, inputSize);
	return ret;
}
#else
static FORCE_INLINE ssize_t lz4_decompress_accel(
	void            *dest,
	ssize_t         outputSize,
	const void      *source,
	ssize_t         inputSize)
{
	return -1;
}
#endif

int z_erofs_unzip_lz4(void *in, void *out, size_t inlen,
		      size_t outlen, bool accel)
{
	ssize_t ret;

	DBG_BUGON(outlen > INT_MAX);
	DBG_BUGON(outlen > SSIZE_MAX);
	DBG_BUGON(inlen > SSIZE_MAX);

	if (accel && lz4_decompress_accel_enable())
		ret = lz4_decompress_accel(out, (ssize_t)outlen,
				in, (ssize_t)inlen);
	else
		ret = customized_lz4_decompress_safe_partial(out, (ssize_t)outlen,
								in, (ssize_t)inlen);

	if (ret >= 0)
		return (int)ret;

	/*
	 * LZ4_decompress_safe will return an error code
	 * (< 0) if decompression failed
	 */
	errln("%s, failed to decompress, in[%p, %zu] outlen[%p, %zu]",
	      __func__, in, inlen, out, outlen);
	WARN_ON(1);
	print_hex_dump(KERN_DEBUG, "raw data [in]: ", DUMP_PREFIX_OFFSET,
		16, 1, in, inlen, true);
	print_hex_dump(KERN_DEBUG, "raw data [out]: ", DUMP_PREFIX_OFFSET,
		16, 1, out, outlen, true);
	return -EIO;
}

