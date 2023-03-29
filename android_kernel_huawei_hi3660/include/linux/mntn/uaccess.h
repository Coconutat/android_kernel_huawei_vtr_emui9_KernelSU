/* Base on arch/arm64/include/asm/uaccess.h
 * For lint e501:
 * Expected signed type
 * affect functions in files:
 *   kbox_snapshot_arm64.c:71   set_fs(KERNEL_DS);
 *   kbox_snapshot_arm64.c:256  set_fs(KERNEL_DS);
 *   kbox_snapshot_arm64.c:282  set_fs(KERNEL_DS);
 *   rdr_common.c:682           set_fs(KERNEL_DS);
 *   rdr_dump_core.c:122        set_fs(KERNEL_DS);
 *   rdr_dump_core.c:190        set_fs(KERNEL_DS);
 *   rdr_dump_core.c:239        set_fs(KERNEL_DS);
 *   rdr_hisi_ap_adapter.c:1320 set_fs(KERNEL_DS);
 *   rdr_logmonitor_core.c:60   set_fs(KERNEL_DS);
 *   rdr_logmonitor_core.c:147  set_fs(KERNEL_DS);
 *   rdr_logmonitor_core.c:518  set_fs(KERNEL_DS);
 */

#ifndef __MNTN_UACCESS_H__
#define __MNTN_UACCESS_H__

#ifdef CONFIG_ARM64

#include <asm/uaccess.h>

/* In arch/arm64/include/asm/uaccess.h the define of
 * KERNEL_DS is (-1UL). pclint will report a warning
 * 501 because of unexpect using of unary negative
 * operator on unsigned types. We replace with
 * (~1UL + 1) which is equal to (-1UL)
 */
#ifdef KERNEL_DS
#undef KERNEL_DS
#define KERNEL_DS (~1UL + 1)
#endif /* KERNEL_DS */

#endif /* CONFIG_ARM64 */

#endif /* __MNTN_UACCESS_H__ */