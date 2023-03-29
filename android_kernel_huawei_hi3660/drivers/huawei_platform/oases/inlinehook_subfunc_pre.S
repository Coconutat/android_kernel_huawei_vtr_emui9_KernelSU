#include "inlinehook_const.h"

#if defined(__aarch64__)

subfunc_pre:
	sub sp, sp, #0x60
	stp x29, x30, [sp]
	mov x29, sp
	stp x0, x1, [sp, #0x18]
	stp x2, x3, [sp, #0x28]
	stp x4, x5, [sp, #0x38]
	stp x6, x7, [sp, #0x48]
	add x0, sp, #0x10
	str x0, [sp, #0x10]
	ldp x0, x1, [sp, #0x10]
	ldp x2, x3, [sp, #0x20]
	ldp x4, x5, [sp, #0x30]
	ldp x6, x7, [sp, #0x40]
	ldr x16, filter_addr
	blr x16
	mov x16, x0
	cbz x0, not_attack_log
	ldr x0, patch_info
	ldr x1, attack_logger
	blr x1
not_attack_log:
	ldp x6, x7, [sp, #0x48]
	ldp x4, x5, [sp, #0x38]
	ldp x2, x3, [sp, #0x28]
	ldp x0, x1, [sp, #0x18]
	ldp x29, x30, [sp]
	add sp, sp, #0x60
	cbz x16, jump_orig
	ldr x0, [sp, #-0x50]
	ret
jump_orig:
	ldr x16, orig_addr
	blr x16
	ldr x16, retn_addr
	br x16
filter_addr:
.quad FILTER_ADDR_MAGIC
orig_addr:
.quad ORIGIN_ADDR_MAGIC
retn_addr:
.quad RETURN_ADDR_MAGIC
patch_info:
.quad PATCH_INFO_CTX_MAGIC
attack_logger:
.quad ATTACK_LOGGER_MAGIC

#elif defined(__arm__)

.arm

subfunc_pre:
	@ assume at most 4 args on stack
	@ assume all argument size are within 32 bits
	@ stack layout:
	@ ret, cpsr, r0-r11, lr, padding, args * 4, = 0x50
	sub sp, #0x50                        @ ret, cpsr
	add r12, sp, #0x48
	stmfd r12, {r0-r11,lr}              @ regs
	mrs r12, cpsr
	str r12, [sp, #0x48]
	ldr r12, [sp, #0x20]                @ save old r3 to stack
	str r12, [sp, #0]
	ldr r12, [sp, #0x50]                @ copy possible args to stack
	str r12, [sp, #4]
	ldr r12, [sp, #0x54]
	str r12, [sp, #8]
	ldr r12, [sp, #0x58]
	str r12, [sp, #0xc]
	add r0, sp, #0x4c                   @ r0 = address of return value
	ldr r1, [sp, #0x14]                 @ map old r0-r2 to new r1-r3
	ldr r2, [sp, #0x18]
	ldr r3, [sp, #0x1c]
	ldr r12, filter_addr
	blx r12
	str r0, [sp, #0x10]
	cmp r0, #0
	beq not_attack_log
	ldr r0, patch_info
	ldr r1, attack_logger
	blx r1
not_attack_log:
	add r12, sp, #0x14
	ldmfd r12, {r0-r11,lr}
	ldr r12, [sp, #0x10]
	cmp r12, #0
	beq jump_orig
	ldr r0, [sp, #0x4c]
	add sp, #0x50
	bx lr
jump_orig:
	ldr r12, [sp, #0x48]
	add sp, #0x50
	msr cpsr_c, r12
place_holder:
	ldr r12, orig_addr
	blx r12
	ldr r12, retn_addr
	bx r12
filter_addr:
.word FILTER_ADDR_MAGIC
orig_addr:
.word ORIGIN_ADDR_MAGIC
retn_addr:
.word RETURN_ADDR_MAGIC
patch_info:
.word PATCH_INFO_CTX_MAGIC
attack_logger:
.word ATTACK_LOGGER_MAGIC

#endif