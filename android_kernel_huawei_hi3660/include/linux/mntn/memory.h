/* Base on arch/arm64/include/asm/memory.h
 * For lint e648:
 * Overflow in computing constant for operation:
 * 'unsigned shift left'
 * affect functions in files:
 *   kbox_meminfoonoom.c:490    (unsigned long)VMALLOC_TOTAL >> 10,
 *   kbox_oom.c:127             page = pfn_to_page(pfn);
 *   kbox_snapshot.c:316        return (struct page *)pfn_to_page(pfn);
 *   rdr_common.c:606           *(pages + i) = phys_to_page(paddr + ((u64)PAGE_SIZE * i));
 */

#ifndef __MNTN_MEMORY_H__
#define __MNTN_MEMORY_H__

#ifdef CONFIG_ARM64

#include <asm/memory.h>

/* In arch/arm64/include/asm/memory.h the define of
 * VA_START is (UL(0xffffffffffffffff) << VA_BITS).
 * pclint will report a warning 648 because of
 * overflow in computing constant with operation
 * 'unsigned shift left'. We replace with
 * (~((UL(1) << VA_BITS) - 1)) which is equal to
 * (UL(0xffffffffffffffff) << VA_BITS)
 */
#ifdef VA_START
#undef VA_START
#define VA_START (~((UL(1) << VA_BITS) - 1))
#endif /* VA_START */

/* In arch/arm64/include/asm/memory.h the define of
 * PAGE_OFFSET is (UL(0xffffffffffffffff) << (VA_BITS - 1)).
 * pclint will report a warning 648 because of
 * overflow in computing constant with operation
 * 'unsigned shift left'. We replace with
 * (~((UL(1) << (VA_BITS - 1)) - 1)) which is equal to
 * (UL(0xffffffffffffffff) << (VA_BITS - 1))
 */
#ifdef PAGE_OFFSET
#undef PAGE_OFFSET
#define PAGE_OFFSET (~((UL(1) << (VA_BITS - 1)) - 1))
#endif /* PAGE_OFFSET */

#endif /* CONFIG_ARM64 */

#endif /* __MNTN_MEMORY_H__ */
