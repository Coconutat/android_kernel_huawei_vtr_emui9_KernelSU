/* Base on linux/compiler.h
 * For warn check
 * expected 'void *' but argument is of type 'volatile int *'
 * affect functions in files:
 *   rdr_common.c:269	return atomic_set(&bb_in_saving, state);
 *   rdr_common.c:280	atomic_set(&bb_in_suspend, 0);
 *   rdr_common.c:287	atomic_set(&bb_in_suspend, 1);
 *   rdr_common.c:307	atomic_set(&bb_in_reboot, 1);
 */

#ifndef __MNTN_COMPILER_H__
#define __MNTN_COMPILER_H__


#include <linux/compiler.h>

/* In linux/compiler.h the define of WRITE_ONCE(x, val) is
 * ({
 *     typeof(x) __val = (val);
 *     __write_once_size(&(x), &__val, sizeof(__val));
 *     __val;
 * })
 * force cast (int *) to (void *).
 */
#ifdef WRITE_ONCE
#undef WRITE_ONCE
#define WRITE_ONCE(x, val) \
({								\
	typeof(x) __val = (val);				\
	__write_once_size(&(x), (void *)&__val, sizeof(__val));	\
	__val;							\
})
#endif /* WRITE_ONCE */

#endif /* __MNTN_COMPILER_H__ */